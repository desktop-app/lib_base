// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include <xcb/xcb.h>

namespace base::Platform::XCB {

class CustomConnection {
public:
	CustomConnection()
	: _connection(xcb_connect(nullptr, nullptr)) {
	}

	~CustomConnection() {
		xcb_disconnect(_connection);
	}

	[[nodiscard]] operator xcb_connection_t*() const {
		return _connection;
	}

	[[nodiscard]] not_null<xcb_connection_t*> get() const {
		return _connection;
	}

private:
	not_null<xcb_connection_t*> _connection;
};

xcb_connection_t *GetConnectionFromQt();
std::optional<xcb_window_t> GetRootWindowFromQt();
std::optional<xcb_timestamp_t> GetAppTimeFromQt();

std::optional<xcb_window_t> GetRootWindow(xcb_connection_t *connection);

std::optional<xcb_atom_t> GetAtom(
		xcb_connection_t *connection,
		const QString &name);

bool IsExtensionPresent(
		xcb_connection_t *connection,
		xcb_extension_t *ext);

std::vector<xcb_atom_t> GetWMSupported(
		xcb_connection_t *connection,
		xcb_window_t root);

std::optional<xcb_window_t> GetSupportingWMCheck(
		xcb_connection_t *connection,
		xcb_window_t root);

bool IsSupportedByWM(const QString &atomName);

} // namespace base::Platform::XCB
