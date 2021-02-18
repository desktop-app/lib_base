// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/integration.h"

#include <QtCore/QLibrary>

extern "C" {
#undef signals
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#define signals public
} // extern "C"

#if defined DESKTOP_APP_USE_PACKAGED && !defined DESKTOP_APP_USE_PACKAGED_LAZY
#define LINK_TO_GTK
#endif // DESKTOP_APP_USE_PACKAGED && !DESKTOP_APP_USE_PACKAGED_LAZY

#ifdef LINK_TO_GTK
#define LOAD_GTK_SYMBOL(lib, name, func) (func = ::func)
#else // LINK_TO_GTK
#define LOAD_GTK_SYMBOL base::Platform::Gtk::LoadSymbol
#endif // !LINK_TO_GTK

namespace base {
namespace Platform {
namespace Gtk {

template <typename Function>
bool LoadSymbol(QLibrary &lib, const char *name, Function &func) {
	func = nullptr;
	if (!lib.isLoaded()) {
		return false;
	}

	func = reinterpret_cast<Function>(lib.resolve(name));
	if (func) {
		return true;
	}

	Integration::Instance().logMessage(
		QString("Error: failed to load '%1' function!").arg(name));

	return false;
}

inline gboolean (*gtk_init_check)(int *argc, char ***argv) = nullptr;
inline const gchar* (*gtk_check_version)(guint required_major, guint required_minor, guint required_micro) = nullptr;
inline GtkSettings* (*gtk_settings_get_default)(void) = nullptr;
inline void (*gdk_set_allowed_backends)(const gchar *backends) = nullptr;

} // namespace Gtk
} // namespace Platform
} // namespace base
