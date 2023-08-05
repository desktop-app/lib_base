/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "base/platform/linux/base_linux_wayland_integration.h"

#include "base/platform/linux/base_linux_wayland_utilities.h"
#include "base/platform/base_platform_info.h"
#include "base/qt_signal_producer.h"
#include "base/flat_map.h"

#include "qwayland-wayland.h"
#include "qwayland-xdg-activation-v1.h"
#include "qwayland-xdg-foreign-unstable-v2.h"
#include "qwayland-idle-inhibit-unstable-v1.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformwindow_p.h>

using namespace QNativeInterface;
using namespace QNativeInterface::Private;
using namespace base::Platform::Wayland;

namespace base {
namespace Platform {
namespace {

class XdgExported : public AutoDestroyer<QtWayland::zxdg_exported_v2> {
public:
	XdgExported(
		not_null<::wl_display*> display,
		not_null<::zxdg_exported_v2*> object)
	: AutoDestroyer(object.get()) {
		wl_display_roundtrip(display.get());
	}

	[[nodiscard]] QString handle() const {
		return _handle;
	}

protected:
	void zxdg_exported_v2_handle(const QString &handle) override {
		_handle = handle;
	}

private:
	QString _handle;
};

class XdgExporter : public Global<QtWayland::zxdg_exporter_v2> {
public:
	using Global::Global;

	base::flat_map<wl_surface*, XdgExported> exporteds;
};

class XdgActivationToken
	: public AutoDestroyer<QtWayland::xdg_activation_token_v1> {
public:
	XdgActivationToken(
		not_null<::wl_display*> display,
		not_null<::xdg_activation_token_v1*> object,
		::wl_surface *surface,
		::wl_seat *seat,
		std::optional<uint32_t> serial,
		const QString &appId)
	: AutoDestroyer(object.get()) {
		if (surface) {
			set_surface(surface);
		}
		if (seat && serial) {
			set_serial(*serial, seat);
		}
		if (!appId.isNull()) {
			set_app_id(appId);
		}
		commit();
		wl_display_roundtrip(display.get());
	}

	QString token() {
		return _token;
	}

protected:
	void xdg_activation_token_v1_done(const QString &token) override {
		_token = token;
	}

private:
	QString _token;
};

class IdleInhibitManager
	: public Global<QtWayland::zwp_idle_inhibit_manager_v1> {
public:
	using Global::Global;

	using Inhibitor = AutoDestroyer<QtWayland::zwp_idle_inhibitor_v1>;
	base::flat_map<wl_surface*, Inhibitor> inhibitors;
};

} // namespace

struct WaylandIntegration::Private : public AutoDestroyer<QtWayland::wl_registry> {
	std::optional<XdgExporter> xdgExporter;
	std::optional<Global<QtWayland::xdg_activation_v1>> xdgActivation;
	std::optional<IdleInhibitManager> idleInhibitManager;
	rpl::lifetime lifetime;

protected:
	void registry_global(
			uint32_t name,
			const QString &interface,
			uint32_t version) override {
		if (interface == qstr("zxdg_exporter_v2")) {
			xdgExporter.emplace(object(), name, version);
		} else if (interface == qstr("xdg_activation_v1")) {
			xdgActivation.emplace(object(), name, version);
		} else if (interface == qstr("zwp_idle_inhibit_manager_v1")) {
			idleInhibitManager.emplace(object(), name, version);
		}
	}

	void registry_global_remove(uint32_t name) override {
		if (xdgExporter && name == xdgExporter->id()) {
			xdgExporter = std::nullopt;
		} else if (xdgActivation && name == xdgActivation->id()) {
			xdgActivation = std::nullopt;
		} else if (idleInhibitManager && name == idleInhibitManager->id()) {
			idleInhibitManager = std::nullopt;
		}
	}
};

WaylandIntegration::WaylandIntegration()
: _private(std::make_unique<Private>()) {
	const auto native = qApp->nativeInterface<QWaylandApplication>();
	if (!native) {
		return;
	}

	const auto display = native->display();
	if (!display) {
		return;
	}

	_private->init(wl_display_get_registry(display));
	wl_display_roundtrip(display);
}

WaylandIntegration::~WaylandIntegration() = default;

WaylandIntegration *WaylandIntegration::Instance() {
	if (!::Platform::IsWayland()) return nullptr;
	static std::optional<WaylandIntegration> instance(std::in_place);
	[[maybe_unused]] static const auto Inited = [] {
		base::qt_signal_producer(
			QGuiApplication::platformNativeInterface(),
			&QObject::destroyed
		) | rpl::start_with_next([] {
			instance = std::nullopt;
		}, instance->_private->lifetime);
		return true;
	}();
	if (!instance) return nullptr;
	return &*instance;
}

QString WaylandIntegration::nativeHandle(QWindow *window) {
	if (!_private->xdgExporter) {
		return {};
	}

	const auto native = qApp->nativeInterface<QWaylandApplication>();
	const auto nativeWindow = window->nativeInterface<QWaylandWindow>();
	if (!native || !nativeWindow) {
		return {};
	}

	const auto display = native->display();
	const auto surface = nativeWindow->surface();

	if (!display || !surface) {
		return {};
	}

	const auto it = _private->xdgExporter->exporteds.find(surface);
	if (it != _private->xdgExporter->exporteds.cend()) {
		return it->second.handle();
	}

	const auto result = _private->xdgExporter->exporteds.emplace(
		surface,
		XdgExported(
			display,
			_private->xdgExporter->export_toplevel(surface)));

	base::qt_signal_producer(
		nativeWindow,
		&QWaylandWindow::surfaceDestroyed
	) | rpl::start_with_next([=] {
		auto it = _private->xdgExporter->exporteds.find(surface);
		if (it != _private->xdgExporter->exporteds.cend()) {
			_private->xdgExporter->exporteds.erase(it);
		}
	}, _private->lifetime);

	return result.first->second.handle();
}

QString WaylandIntegration::activationToken(const QString &appId) {
	if (!_private->xdgActivation) {
		return {};
	}

	const auto window = QGuiApplication::focusWindow();
	if (!window) {
		return {};
	}

	const auto native = qApp->nativeInterface<QWaylandApplication>();
	const auto nativeWindow = window->nativeInterface<QWaylandWindow>();
	if (!native || !nativeWindow) {
		return {};
	}

	const auto display = native->display();
	const auto surface = nativeWindow->surface();
	const auto seat = native->lastInputSeat();
	if (!display || !surface || !seat) {
		return {};
	}

	return XdgActivationToken(
		display,
		_private->xdgActivation->get_activation_token(),
		surface,
		seat,
		native->lastInputSerial(),
		appId
	).token();
}

void WaylandIntegration::preventDisplaySleep(bool prevent, QWindow *window) {
	if (!_private->idleInhibitManager) {
		return;
	}

	const auto native = window->nativeInterface<QWaylandWindow>();
	if (!native) {
		return;
	}

	const auto surface = native->surface();
	if (!surface) {
		return;
	}

	const auto deleter = [=] {
		auto it = _private->idleInhibitManager->inhibitors.find(surface);
		if (it != _private->idleInhibitManager->inhibitors.cend()) {
			_private->idleInhibitManager->inhibitors.erase(it);
		}
	};

	if (!prevent) {
		deleter();
		return;
	}

	if (_private->idleInhibitManager->inhibitors.contains(surface)) {
		return;
	}

	const auto inhibitor = _private->idleInhibitManager->create_inhibitor(
		surface);

	if (!inhibitor) {
		return;
	}

	_private->idleInhibitManager->inhibitors.emplace(surface, inhibitor);

	base::qt_signal_producer(
		native,
		&QWaylandWindow::surfaceDestroyed
	) | rpl::start_with_next(deleter, _private->lifetime);
}

} // namespace Platform
} // namespace base
