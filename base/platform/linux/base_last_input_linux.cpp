// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_last_input_linux.h"

#include "base/debug_log.h"

#include "base/platform/linux/base_linux_library.h"
#include "base/platform/linux/base_linux_xcb_utilities.h"

// Declarations from the xcb-screensaver headers (X11 license), so that
// there is no build-time dependency on it.
extern "C" {

typedef struct xcb_screensaver_query_info_cookie_t {
    unsigned int sequence;
} xcb_screensaver_query_info_cookie_t;

typedef struct xcb_screensaver_query_info_reply_t {
    uint8_t      response_type;
    uint8_t      state;
    uint16_t     sequence;
    uint32_t     length;
    xcb_window_t saver_window;
    uint32_t     ms_until_server;
    uint32_t     ms_since_user_input;
    uint32_t     event_mask;
    uint8_t      kind;
    uint8_t      pad0[7];
} xcb_screensaver_query_info_reply_t;

} // extern "C"

#include <mutteridlemonitor/mutteridlemonitor.hpp>

namespace base::Platform {
namespace {

using namespace gi::repository;

using namespace XCB::Library;

[[nodiscard]] void *LoadScreenSaverSymbol(const char *name) {
	static const auto Library = LoadLibrary(
		"libxcb-screensaver.so.0",
		RTLD_NODELETE);
	return Library ? LoadSymbolGeneric(Library, name) : nullptr;
}

template <typename Function>
[[nodiscard]] Function *LoadScreenSaverSymbol(const char *name) {
	return reinterpret_cast<Function*>(LoadScreenSaverSymbol(name));
}

std::optional<crl::time> XCBLastUserInputTime() {
	static const auto xcb_screensaver_id = static_cast<xcb_extension_t*>(
		LoadScreenSaverSymbol("xcb_screensaver_id"));
	static const auto xcb_screensaver_query_info = LoadScreenSaverSymbol<
		xcb_screensaver_query_info_cookie_t(
			xcb_connection_t*,
			xcb_drawable_t)>("xcb_screensaver_query_info");
	static const auto xcb_screensaver_query_info_reply
		= LoadScreenSaverSymbol<xcb_screensaver_query_info_reply_t*(
			xcb_connection_t*,
			xcb_screensaver_query_info_cookie_t,
			xcb_generic_error_t**)>("xcb_screensaver_query_info_reply");

	if (!xcb_screensaver_id
		|| !xcb_screensaver_query_info
		|| !xcb_screensaver_query_info_reply) {
		return std::nullopt;
	}

	const XCB::Connection connection;
	if (!connection || xcb_connection_has_error(connection)) {
		return std::nullopt;
	}

	if (!XCB::IsExtensionPresent(connection, xcb_screensaver_id)) {
		return std::nullopt;
	}

	const auto root = XCB::GetRootWindow(connection);
	if (!root) {
		return std::nullopt;
	}

	const auto cookie = xcb_screensaver_query_info(
		connection,
		root);

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

std::optional<crl::time> MutterDBusLastUserInputTime() {
	static auto NotSupported = false;
	if (NotSupported) {
		return std::nullopt;
	}

	auto interface = MutterIdleMonitor::IdleMonitor(
		MutterIdleMonitor::IdleMonitorProxy::new_for_bus_sync(
			Gio::BusType::SESSION_,
			Gio::DBusProxyFlags::DO_NOT_AUTO_START_AT_CONSTRUCTION_,
			"org.gnome.Mutter.IdleMonitor",
			"/org/gnome/Mutter/IdleMonitor/Core",
			nullptr));

	if (!interface) {
		NotSupported = true;
		return std::nullopt;
	}

	const auto result = interface.call_get_idletime_sync();
	if (!result) {
		NotSupported = true;
		return std::nullopt;
	}

	return (crl::now() - static_cast<crl::time>(std::get<1>(*result)));
}

} // namespace

std::optional<crl::time> LastUserInputTime() {
	const auto xcbResult = XCBLastUserInputTime();
	if (xcbResult.has_value()) {
		return xcbResult;
	}

	const auto mutterResult = MutterDBusLastUserInputTime();
	if (mutterResult.has_value()) {
		return mutterResult;
	}

	return std::nullopt;
}

} // namespace base::Platform
