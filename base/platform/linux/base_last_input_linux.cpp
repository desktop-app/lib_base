// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_last_input_linux.h"

#include "base/platform/base_platform_info.h"
#include "base/debug_log.h"

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
#include "base/platform/linux/base_linux_xcb_utilities.h"

#include <xcb/screensaver.h>
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

#include <xdgscreensaver/xdgscreensaver.hpp>
#include <mutteridlemonitor/mutteridlemonitor.hpp>

namespace base::Platform {
namespace {

using namespace gi::repository;

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
std::optional<crl::time> XCBLastUserInputTime() {
	const XCB::Connection connection;
	if (!connection || xcb_connection_has_error(connection)) {
		return std::nullopt;
	}

	if (!XCB::IsExtensionPresent(connection, &xcb_screensaver_id)) {
		return std::nullopt;
	}

	const auto root = XCB::GetRootWindow(connection);
	if (!root.has_value()) {
		return std::nullopt;
	}

	const auto cookie = xcb_screensaver_query_info(
		connection,
		*root);

	const auto reply = XCB::MakeReplyPointer(
		xcb_screensaver_query_info_reply(
			connection,
			cookie,
			nullptr));

	if (!reply) {
		return std::nullopt;
	}

	return (crl::now() - static_cast<crl::time>(reply->ms_since_user_input));
}
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

std::optional<crl::time> FreedesktopDBusLastUserInputTime() {
	auto interface = XdgScreenSaver::ScreenSaver(
		XdgScreenSaver::ScreenSaverProxy::new_for_bus_sync(
			Gio::BusType::SESSION_,
			Gio::DBusProxyFlags::NONE_,
			"org.freedesktop.ScreenSaver",
			"/org/freedesktop/ScreenSaver",
			nullptr));

	if (!interface) {
		return std::nullopt;
	}

	const auto result = interface.call_get_session_idle_time_sync();
	if (!result) {
		return std::nullopt;
	}

	return (crl::now() - static_cast<crl::time>(std::get<1>(*result)));
}

std::optional<crl::time> MutterDBusLastUserInputTime() {
	auto interface = MutterIdleMonitor::IdleMonitor(
		MutterIdleMonitor::IdleMonitorProxy::new_for_bus_sync(
			Gio::BusType::SESSION_,
			Gio::DBusProxyFlags::NONE_,
			"org.gnome.Mutter.IdleMonitor",
			"/org/gnome/Mutter/IdleMonitor/Core",
			nullptr));

	if (!interface) {
		return std::nullopt;
	}

	const auto result = interface.call_get_idletime_sync();
	if (!result) {
		return std::nullopt;
	}

	return (crl::now() - static_cast<crl::time>(std::get<1>(*result)));
}

} // namespace

std::optional<crl::time> LastUserInputTime() {
#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
	if (::Platform::IsX11()) {
		const auto xcbResult = XCBLastUserInputTime();
		if (xcbResult.has_value()) {
			return xcbResult;
		}
	}
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

	const auto freedesktopResult = FreedesktopDBusLastUserInputTime();
	if (freedesktopResult.has_value()) {
		return freedesktopResult;
	}

	const auto mutterResult = MutterDBusLastUserInputTime();
	if (mutterResult.has_value()) {
		return mutterResult;
	}

	return std::nullopt;
}

} // namespace base::Platform
