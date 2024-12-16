// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_process_linux.h"

#include "base/platform/base_platform_info.h"

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
#include "base/platform/linux/base_linux_xcb_utilities.h"
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

#include <QtGui/QGuiApplication>

namespace base::Platform {
namespace {

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
void XCBActivateWindow(WId window) {
	const XCB::Connection connection;
	if (!connection || xcb_connection_has_error(connection)) {
		return;
	}

	const auto root = XCB::GetRootWindow(connection);
	if (!root) {
		return;
	}

	const auto timestamp = XCB::GetTimestamp(connection);
	if (!timestamp) {
		return;
	}

	const auto activeWindowAtom = XCB::GetAtom(
		connection,
		"_NET_ACTIVE_WINDOW");

	if (!activeWindowAtom) {
		return;
	}

	const auto focusWindow = QGuiApplication::focusWindow();

	// map the window first
	free(
		xcb_request_check(
			connection,
			xcb_map_window_checked(connection, window)));

	// now raise (restack) the window
	uint32_t values[] = { XCB_STACK_MODE_ABOVE };
	free(
		xcb_request_check(
			connection,
			xcb_configure_window_checked(
				connection,
				window,
				XCB_CONFIG_WINDOW_STACK_MODE,
				values)));

	// and, finally, make it the active window
	xcb_client_message_event_t xev;
	xev.response_type = XCB_CLIENT_MESSAGE;
	xev.format = 32;
	xev.sequence = 0;
	xev.window = window;
	xev.type = activeWindowAtom;
	xev.data.data32[0] = 1; // source: 1=application 2=pager
	xev.data.data32[1] = timestamp; // timestamp
	xev.data.data32[2] = focusWindow // currently active window
		? focusWindow->winId()
		: XCB_NONE;
	xev.data.data32[3] = 0;
	xev.data.data32[4] = 0;

	free(
		xcb_request_check(
			connection,
			xcb_send_event_checked(
				connection,
				false,
				root,
				XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
					| XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
				reinterpret_cast<const char *>(&xev))));
}
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

} // namespace

void ActivateProcessWindow(int64 pid, WId windowId) {
#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
	if (::Platform::IsX11()) {
		XCBActivateWindow(windowId);
	}
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION
}

void ActivateThisProcessWindow(WId windowId) {
#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
	if (::Platform::IsX11()) {
		XCBActivateWindow(windowId);
	}
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION
}

} // namespace base::Platform
