// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_process_linux.h"

#include "base/platform/linux/base_info_linux.h"
#include "base/platform/linux/base_xcb_utilities_linux.h"

namespace base::Platform {
namespace {

void XCBActivateWindow(WId window) {
	const auto connection = XCB::GetConnectionFromQt();
	if (!connection) {
		return;
	}

	const auto root = XCB::GetRootWindowFromQt();
	if (!root.has_value()) {
		return;
	}

	const auto activeWindowAtom = XCB::GetAtom(
		connection,
		"_NET_ACTIVE_WINDOW");

	if (!activeWindowAtom.has_value()) {
		return;
	}

	// map the window first
	xcb_map_window(connection, window);

	// now raise (restack) the window
	uint32_t values[] = { XCB_STACK_MODE_ABOVE };
	xcb_configure_window(
		connection,
		window,
		XCB_CONFIG_WINDOW_STACK_MODE,
		values);

	// and, finally, make it the active window
	xcb_client_message_event_t xev;
	xev.response_type = XCB_CLIENT_MESSAGE;
	xev.format = 32;
	xev.sequence = 0;
	xev.window = window;
	xev.type = *activeWindowAtom;
	xev.data.data32[0] = 1UL; // source: 1=application 2=pager
	xev.data.data32[1] = 1UL; // timestamp
	xev.data.data32[2] = 0UL; // currently active window (none)
	xev.data.data32[3] = 0UL;
	xev.data.data32[4] = 0UL;

	xcb_send_event(
		connection,
		true,
		*root,
		XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
		reinterpret_cast<const char *>(&xev));
}

} // namespace

void ActivateProcessWindow(int64 pid, WId windowId) {
	if (!::Platform::IsWayland()) {
		XCBActivateWindow(windowId);
	}
}

void ActivateThisProcessWindow(WId windowId) {
	if (!::Platform::IsWayland()) {
		XCBActivateWindow(windowId);
	}
}

} // namespace base::Platform
