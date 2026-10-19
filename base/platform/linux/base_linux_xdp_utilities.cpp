// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_linux_xdp_utilities.h"

#include "base/platform/base_platform_info.h"

#include <xdpsettings/xdpsettings.hpp>

#include <QtCore/QEventLoop>
#include <QtCore/QFutureWatcher>
#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0) && !defined DESKTOP_APP_DISABLE_PRIVATE_QT_API
#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
#include <private/qdesktopunixservices_p.h>
#else // Qt >= 6.9.0
#include <private/qgenericunixservices_p.h>
#endif // Qt < 6.9.0
#elif __has_include(<KWaylandExtras>) // Qt >= 6.5.0 && !DESKTOP_APP_DISABLE_PRIVATE_QT_API
#include <KWaylandExtras>
#endif // (Qt < 6.5.0 || DESKTOP_APP_DISABLE_PRIVATE_QT_API) && __has_include(<KWaylandExtras>)

#include <sstream>

namespace base::Platform::XDP {
namespace {

using namespace gi::repository;
namespace GObject = gi::repository::GObject;

#if (QT_VERSION < QT_VERSION_CHECK(6, 5, 0) || defined DESKTOP_APP_DISABLE_PRIVATE_QT_API) && __has_include(<KWaylandExtras>)
constexpr auto kUnexportOnHideProperty = "_td_unexport_on_hide";

[[nodiscard]] std::string WaylandParentWindowID(not_null<QWindow*> window) {
	// Export anew after hide like Qt does: the old xdg_toplevel is gone.
	if (!window->property(kUnexportOnHideProperty).toBool()) {
		window->setProperty(kUnexportOnHideProperty, true);
		QObject::connect(window, &QWindow::visibleChanged, window, [=](
				bool visible) {
			if (!visible) {
				KWaylandExtras::unexportToplevel(window);
			}
		});
	}

	auto future = KWaylandExtras::exportToplevel(window);
	if (!future.isFinished()) {
		QEventLoop loop;
		QFutureWatcher<QString> watcher;
		QObject::connect(
			&watcher,
			&QFutureWatcherBase::finished,
			&loop,
			&QEventLoop::quit);
		watcher.setFuture(future);
		loop.exec();
	}

	return future.resultCount()
		? ("wayland:" + future.result()).toStdString()
		: std::string();
}
#endif // (Qt < 6.5.0 || DESKTOP_APP_DISABLE_PRIVATE_QT_API) && __has_include(<KWaylandExtras>)

} // namespace

std::string ParentWindowID() {
	return ParentWindowID(QGuiApplication::focusWindow());
}

std::string ParentWindowID(QWindow *window) {
	if (!window) {
		return {};
	}

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0) && !defined DESKTOP_APP_DISABLE_PRIVATE_QT_API
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
	using QDesktopUnixServices = QGenericUnixServices;
#endif // Qt < 6.9.0
	if (const auto services = dynamic_cast<QDesktopUnixServices*>(
			QGuiApplicationPrivate::platformIntegration()->services())) {
		return services->portalWindowIdentifier(window).toStdString();
	}
#elif __has_include(<KWaylandExtras>) // Qt >= 6.5.0 && !DESKTOP_APP_DISABLE_PRIVATE_QT_API
	if (::Platform::IsWayland()) {
		return WaylandParentWindowID(window);
	}
#endif // (Qt < 6.5.0 || DESKTOP_APP_DISABLE_PRIVATE_QT_API) && __has_include(<KWaylandExtras>)

	if (::Platform::IsX11()) {
		std::stringstream result;
		result << "x11:" << std::hex << window->winId();
		return result.str();
	}

	return {};
}

gi::result<GLib::Variant> ReadSetting(
		const std::string &group,
		const std::string &key) {
	auto interface = gi::result<XdpSettings::Settings>(
		XdpSettings::SettingsProxy::new_for_bus_sync(
			Gio::BusType::SESSION_,
			Gio::DBusProxyFlags::NONE_,
			kService,
			kObjectPath));

	if (!interface) {
		return make_unexpected(std::move(interface.error()));
	}

	auto result = interface->call_read_one_sync(group, key);
	if (!result) {
		return make_unexpected(std::move(result.error()));
	}

	return std::get<1>(*result).get_variant();
}

class SettingWatcher::Private {
public:
	XdpSettings::Settings interface;
};

SettingWatcher::SettingWatcher(
		Fn<void(
			const std::string &,
			const std::string &,
			GLib::Variant)> callback)
: _private(std::make_unique<Private>()) {
	XdpSettings::SettingsProxy::new_for_bus(
		Gio::BusType::SESSION_,
		Gio::DBusProxyFlags::NONE_,
		kService,
		kObjectPath,
		crl::guard(this, [=](GObject::Object, Gio::AsyncResult res) {
			_private->interface = XdpSettings::Settings(
				XdpSettings::SettingsProxy::new_for_bus_finish(res, nullptr));

			if (!_private->interface) {
				return;
			}

			_private->interface.signal_setting_changed().connect([=](
					XdpSettings::Settings,
					std::string group,
					std::string key,
					GLib::Variant value) {
				callback(group, key, value.get_variant());
			});
		}));
}

SettingWatcher::~SettingWatcher() = default;

} // namespace base::Platform::XDP
