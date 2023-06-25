// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/base_platform_network_reachability.h"

#include <giomm.h>

namespace base::Platform {
namespace {

class NetworkReachabilityImpl
	: public NetworkReachability
	, public sigc::trackable {
public:
	NetworkReachabilityImpl()
	: _monitor(Gio::NetworkMonitor::get_default())
	, _available(_monitor->get_network_available()) {
		_monitor->property_network_available(
		).signal_changed(
		).connect(sigc::track_object([=] {
			_available = _monitor->get_network_available();
		}, *this));
	}

	rpl::producer<bool> availableValue() const override {
		return _available.value();
	}

private:
	Glib::RefPtr<Gio::NetworkMonitor> _monitor;
	rpl::variable<bool> _available;
};

} // namespace

// glib is better on linux due to portal support
std::unique_ptr<NetworkReachability> NetworkReachability::Create() {
	// crashes on 2.46.0...2.60.0, but not on >=2.56.3, >=2.58.1 (fix backported)
	if ((!glib_check_version(2, 56, 0) && glib_check_version(2, 56, 3))
		|| (!glib_check_version(2, 58, 0) && glib_check_version(2, 58, 1))
		|| (glib_check_version(2, 60, 0)
			&& glib_check_version(2, 58, 0)
			&& glib_check_version(2, 56, 0)
			&& !glib_check_version(2, 46, 0))) {
		return nullptr;
	}

	return std::make_unique<NetworkReachabilityImpl>();
}

} // namespace base::Platform
