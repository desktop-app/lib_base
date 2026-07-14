// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_last_input_linux.h"

#include "base/debug_log.h"
#include "base/platform/linux/base_linux_xcb_utilities.h"

#include <sys/uio.h>

// Declarations from the xcb-screensaver headers (X11 license), so that
// there is no build-time dependency on it.
extern "C" {

typedef struct xcb_screensaver_query_info_request_t {
    uint8_t        major_opcode;
    uint8_t        minor_opcode;
    uint16_t       length;
    xcb_drawable_t drawable;
} xcb_screensaver_query_info_request_t;

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

std::optional<crl::time> XCBLastUserInputTime() {
	const XCB::Connection connection;
	if (!connection || xcb_connection_has_error(connection)) {
		return std::nullopt;
	}

	// The object must persist: libxcb lazily assigns a global id into it
	// and keys the per-connection extension cache by that id.
	static xcb_extension_t xcb_screensaver_id = { "MIT-SCREEN-SAVER", 0 };

	if (!XCB::IsExtensionPresent(connection, &xcb_screensaver_id)) {
		return std::nullopt;
	}

	const auto root = XCB::GetRootWindow(connection);
	if (!root) {
		return std::nullopt;
	}

	static const xcb_protocol_request_t request = {
		.count = 2,
		.ext = &xcb_screensaver_id,
		.opcode = 1, // XCB_SCREENSAVER_QUERY_INFO
		.isvoid = 0,
	};

	xcb_screensaver_query_info_request_t body;
	body.drawable = root;

	struct iovec parts[4];
	parts[2].iov_base = &body;
	parts[2].iov_len = sizeof(body);
	parts[3].iov_base = nullptr;
	parts[3].iov_len = -parts[2].iov_len & 3;

	const auto reply = XCB::MakeReplyPointer(
		reinterpret_cast<xcb_screensaver_query_info_reply_t*>(
			xcb_wait_for_reply(
				connection,
				xcb_send_request(
					connection,
					XCB_REQUEST_CHECKED,
					parts + 2,
					&request),
				nullptr)));

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
