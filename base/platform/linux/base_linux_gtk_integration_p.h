// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/platform/linux/base_linux_library.h"

extern "C" {
#include <gtk/gtk.h>
#include <gdk/gdk.h>
} // extern "C"

#define LOAD_GTK_SYMBOL LOAD_LIBRARY_SYMBOL

namespace base {
namespace Platform {
namespace Gtk {

inline bool LoadGtkLibrary(
		QLibrary &lib,
		const char *name,
		std::optional<int> version = std::nullopt) {
	return LoadLibrary(lib, name, version);
}

inline gboolean (*gtk_init_check)(int *argc, char ***argv) = nullptr;
inline const gchar* (*gtk_check_version)(guint required_major, guint required_minor, guint required_micro) = nullptr;
inline GtkSettings* (*gtk_settings_get_default)(void) = nullptr;
inline void (*gdk_set_allowed_backends)(const gchar *backends) = nullptr;

} // namespace Gtk
} // namespace Platform
} // namespace base
