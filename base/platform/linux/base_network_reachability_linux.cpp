// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/base_platform_network_reachability.h"

#include <giomm.h>

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

	[[maybe_unused]] static const auto Inited = [] {
		Gio::NetworkMonitor::get_default(
		)->property_network_available(
		).signal_changed(
		).connect([] {
			NotifyNetworkAvailableChanged();
		});
		return true;
	}();

	return Gio::NetworkMonitor::get_default()->get_network_available();
}

} // namespace base::Platform
