// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

namespace base::Platform {

[[nodiscard]] std::optional<bool> NetworkAvailable();
[[nodiscard]] rpl::producer<> NetworkAvailableChanged();
void NotifyNetworkAvailableChanged();
[[nodiscard]] inline bool NetworkAvailableSupported() {
	return NetworkAvailable().has_value();
}

} // namespace base::Platform

#ifdef Q_OS_MAC
#include "base/platform/mac/base_network_reachability_mac.h"
#elif defined Q_OS_UNIX // Q_OS_MAC
#include "base/platform/linux/base_network_reachability_linux.h"
#elif defined Q_OS_WIN // Q_OS_MAC || Q_OS_UNIX
#include "base/platform/win/base_network_reachability_win.h"
#endif // Q_OS_MAC || Q_OS_UNIX || Q_OS_WIN
