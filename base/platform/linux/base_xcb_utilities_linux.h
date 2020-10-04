// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include <xcb/xcb.h>

namespace base::Platform::XCB {

xcb_connection_t *GetConnectionFromQt();
std::optional<xcb_window_t> GetRootWindowFromQt();

std::optional<xcb_atom_t> GetAtom(
		xcb_connection_t *connection,
		const QString &name);

bool IsExtensionPresent(
		xcb_connection_t *connection,
		xcb_extension_t *ext);

std::vector<xcb_atom_t> GetWMSupported(xcb_connection_t *connection);

} // namespace base::Platform::XCB
