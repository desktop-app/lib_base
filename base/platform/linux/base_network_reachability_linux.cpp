// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/base_platform_network_reachability.h"

#include <gio/gio.h>

namespace base::Platform {

// glib is better on linux due to portal support
std::optional<bool> NetworkAvailable() {
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
