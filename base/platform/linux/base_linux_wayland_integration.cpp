// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_linux_wayland_integration.h"

#include "base/platform/linux/base_linux_wayland_utilities.h"
#include "base/qt_signal_producer.h"
#include "base/flat_map.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformwindow_p.h>

#include <qwayland-wayland.h>
#include <qwayland-idle-inhibit-unstable-v1.h>

using QWlApp = QNativeInterface::QWaylandApplication;
using namespace QNativeInterface::Private;
using namespace base::Platform::Wayland;

namespace base {
namespace Platform {
namespace {

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

struct WaylandIntegration::Private
		: public AutoDestroyer<QtWayland::wl_registry> {
	Private(not_null<QWlApp*> native)
	: AutoDestroyer(wl_display_get_registry(native->display()))
	, display(native->display()) {
		wl_display_roundtrip(display);
	}

	const not_null<wl_display*> display;
	std::optional<IdleInhibitManager> idleInhibitManager;

protected:
	void registry_global(
			uint32_t name,
			const QString &interface,
			uint32_t version) override {
		if (interface == qstr("zwp_idle_inhibit_manager_v1")) {
			idleInhibitManager.emplace(object(), name, version);
		}
	}

	void registry_global_remove(uint32_t name) override {
		if (idleInhibitManager && name == idleInhibitManager->id()) {
			idleInhibitManager = std::nullopt;
		}
	}
};

WaylandIntegration::WaylandIntegration()
: _private(std::make_unique<Private>(qApp->nativeInterface<QWlApp>())) {
}

WaylandIntegration::~WaylandIntegration() = default;

WaylandIntegration *WaylandIntegration::Instance() {
	const auto native = qApp->nativeInterface<QWlApp>();
	if (!native) return nullptr;
	static std::optional<WaylandIntegration> instance;
	if (instance && native->display() != instance->_private->display) {
		instance.reset();
	}
	if (!instance) {
		instance.emplace();
		base::qt_signal_producer(
			QGuiApplication::platformNativeInterface(),
			&QObject::destroyed
		) | rpl::start_with_next([] {
			instance = std::nullopt;
		}, instance->_private->lifetime());
	}
	return &*instance;
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
