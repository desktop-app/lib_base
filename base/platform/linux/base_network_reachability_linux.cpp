// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_network_reachability_linux.h"

#include <gio/gio.h>

namespace base::Platform {

// glib is better on linux due to portal support
std::optional<bool> NetworkAvailable() {
	// crashes on 2.46.0...2.60.0, but not on >=2.56.3, >=2.58.1 (fix backported)
	if ((!glib_check_version(2, 56, 0) && glib_check_version(2, 56, 3))
		|| (!glib_check_version(2, 58, 0) && glib_check_version(2, 58, 1))
		|| (glib_check_version(2, 60, 0)
			&& glib_check_version(2, 58, 0)
			&& glib_check_version(2, 56, 0)
			&& !glib_check_version(2, 46, 0))) {
		return std::nullopt;
	}

	static const auto Inited = [] {
		g_signal_connect(
			g_network_monitor_get_default(),
			"notify::network-available",
			G_CALLBACK(NotifyNetworkAvailableChanged),
			nullptr);
		return true;
	}();

	return g_network_monitor_get_network_available(
		g_network_monitor_get_default());
}

} // namespace base::Platform
