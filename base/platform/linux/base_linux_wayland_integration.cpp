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

#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformwindow_p.h>

#include <qwayland-wayland.h>
#include <qwayland-xdg-activation-v1.h>
#include <qwayland-idle-inhibit-unstable-v1.h>

using namespace QNativeInterface;
using namespace QNativeInterface::Private;
using namespace base::Platform::Wayland;

namespace base {
namespace Platform {
namespace {

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

	class Inhibitor;
	base::flat_map<QWaylandWindow*, std::unique_ptr<Inhibitor>> inhibitors;
};

class IdleInhibitManager::Inhibitor {
public:
	Inhibitor(
			not_null<IdleInhibitManager*> manager,
			not_null<QWaylandWindow*> window) {
		if (const auto surface = window->surface()) {
			_object = manager->create_inhibitor(surface);
		}

		base::qt_signal_producer(
			window.get(),
			&QWaylandWindow::surfaceCreated
		) | rpl::start_with_next([=] {
			_object = manager->create_inhibitor(window->surface());
		}, _lifetime);

		base::qt_signal_producer(
			window.get(),
			&QWaylandWindow::surfaceDestroyed
		) | rpl::start_with_next([=] {
			_object = {};
		}, _lifetime);
	}

	Inhibitor(Inhibitor &&other) = delete;
	Inhibitor &operator=(Inhibitor &&other) = delete;

	[[nodiscard]] rpl::lifetime &lifetime() {
		return _lifetime;
	}

private:
	AutoDestroyer<QtWayland::zwp_idle_inhibitor_v1> _object;
	rpl::lifetime _lifetime;
};

} // namespace

struct WaylandIntegration::Private : public AutoDestroyer<QtWayland::wl_registry> {
	std::optional<Global<QtWayland::xdg_activation_v1>> xdgActivation;
	std::optional<IdleInhibitManager> idleInhibitManager;

protected:
	void registry_global(
			uint32_t name,
			const QString &interface,
			uint32_t version) override {
		if (interface == qstr("xdg_activation_v1")) {
			xdgActivation.emplace(object(), name, version);
		} else if (interface == qstr("zwp_idle_inhibit_manager_v1")) {
			idleInhibitManager.emplace(object(), name, version);
		}
	}

	void registry_global_remove(uint32_t name) override {
		if (xdgActivation && name == xdgActivation->id()) {
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
		}, instance->_private->lifetime());
		return true;
	}();
	if (!instance) return nullptr;
	return &*instance;
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

	const auto deleter = [=] {
		auto it = _private->idleInhibitManager->inhibitors.find(native);
		if (it != _private->idleInhibitManager->inhibitors.cend()) {
			_private->idleInhibitManager->inhibitors.erase(it);
		}
	};

	if (!prevent) {
		deleter();
		return;
	}

	if (_private->idleInhibitManager->inhibitors.contains(native)) {
		return;
	}

	const auto result = _private->idleInhibitManager->inhibitors.emplace(
		native,
		std::make_unique<IdleInhibitManager::Inhibitor>(
			&*_private->idleInhibitManager,
			native));

	base::qt_signal_producer(
		native,
		&QObject::destroyed
	) | rpl::start_with_next(deleter, result.first->second->lifetime());
}

} // namespace Platform
} // namespace base
