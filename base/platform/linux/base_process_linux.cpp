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
std::optional<uint32> XCBCurrentWorkspace() {
	const auto connection = XCB::GetConnectionFromQt();
	if (!connection) {
		return std::nullopt;
	}

	const auto root = XCB::GetRootWindowFromQt();
	if (!root.has_value()) {
		return std::nullopt;
	}

	const auto currentDesktopAtom = XCB::GetAtom(
		connection,
		"_NET_CURRENT_DESKTOP");

	if (!currentDesktopAtom.has_value()) {
		return std::nullopt;
	}

	const auto cookie = xcb_get_property(
		connection,
		false,
		*root,
		*currentDesktopAtom,
		XCB_ATOM_CARDINAL,
		0,
		1024);

	auto reply = xcb_get_property_reply(
		connection,
		cookie,
		nullptr);

	if (!reply) {
		return std::nullopt;
	}

	const auto result = (reply->type == XCB_ATOM_CARDINAL)
		? std::make_optional(
			*reinterpret_cast<ulong*>(xcb_get_property_value(reply)))
		: std::nullopt;

	free(reply);

	return result;
}

std::optional<uint32> XCBGetWindowWorkspace(WId window) {
	const auto connection = XCB::GetConnectionFromQt();
	if (!connection) {
		return std::nullopt;
	}

	const auto desktopAtom = XCB::GetAtom(
		connection,
		"_NET_WM_DESKTOP");

	if (!desktopAtom.has_value()) {
		return std::nullopt;
	}

	const auto cookie = xcb_get_property(
		connection,
		false,
		window,
		*desktopAtom,
		XCB_ATOM_CARDINAL,
		0,
		1024);

	auto reply = xcb_get_property_reply(
		connection,
		cookie,
		nullptr);
	
	if (!reply) {
		return std::nullopt;
	}

	const auto result = (reply->type == XCB_ATOM_CARDINAL)
		? std::make_optional(
			*reinterpret_cast<ulong*>(xcb_get_property_value(reply)))
		: std::nullopt;

	free(reply);

	return result;
}

void XCBMoveWindowToWorkspace(WId window, uint32 workspace) {
	const auto connection = XCB::GetConnectionFromQt();
	if (!connection) {
		return;
	}

	const auto root = XCB::GetRootWindowFromQt();
	if (!root.has_value()) {
		return;
	}

	const auto desktopAtom = XCB::GetAtom(
		connection,
		"_NET_WM_DESKTOP");

	if (!desktopAtom.has_value()) {
		return;
	}

	xcb_client_message_event_t xev;
	xev.response_type = XCB_CLIENT_MESSAGE;
	xev.format = 32;
	xev.sequence = 0;
	xev.window = window;
	xev.type = *desktopAtom;
	xev.data.data32[0] = workspace;
	xev.data.data32[1] = 0;
	xev.data.data32[2] = 0;
	xev.data.data32[3] = 0;
	xev.data.data32[4] = 0;

	xcb_send_event(
		connection,
		false,
		*root,
		XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
			| XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
		reinterpret_cast<const char *>(&xev));
}

void XCBMoveWindowToCurrentWorkspace(WId window) {
	if (const auto windowWorkspace = XCBGetWindowWorkspace(window);
		*windowWorkspace != 0xFFFFFFFF) {
		if (const auto currentWorkspace = XCBCurrentWorkspace();
			*currentWorkspace != *windowWorkspace) {
			XCBMoveWindowToWorkspace(window, *currentWorkspace);
		}
	}
}

void XCBActivateWindow(WId window) {
	const auto connection = XCB::GetConnectionFromQt();
	if (!connection) {
		return;
	}

	const auto root = XCB::GetRootWindowFromQt();
	if (!root.has_value()) {
		return;
	}

	const auto appTime = XCB::GetAppTimeFromQt();
	if (!appTime.has_value()) {
		return;
	}

	const auto activeWindowAtom = XCB::GetAtom(
		connection,
		"_NET_ACTIVE_WINDOW");

	if (!activeWindowAtom.has_value()) {
		return;
	}

	const auto focusWindow = QGuiApplication::focusWindow();

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
	xev.data.data32[0] = 1; // source: 1=application 2=pager
	xev.data.data32[1] = *appTime; // timestamp
	xev.data.data32[2] = focusWindow // currently active window
		? focusWindow->winId()
		: XCB_NONE;
	xev.data.data32[3] = 0;
	xev.data.data32[4] = 0;

	xcb_send_event(
		connection,
		false,
		*root,
		XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
			| XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
		reinterpret_cast<const char *>(&xev));
}
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

} // namespace

void ActivateProcessWindow(int64 pid, WId windowId) {
#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
	if (::Platform::IsX11()) {
		XCBMoveWindowToCurrentWorkspace(windowId);
		XCBActivateWindow(windowId);
	}
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION
}

void ActivateThisProcessWindow(WId windowId) {
#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
	if (::Platform::IsX11()) {
		XCBMoveWindowToCurrentWorkspace(windowId);
		XCBActivateWindow(windowId);
	}
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION
}

} // namespace base::Platform
