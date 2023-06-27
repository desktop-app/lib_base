// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

namespace base::Platform {

class NetworkReachability {
public:
	virtual ~NetworkReachability() = default;

	static std::unique_ptr<NetworkReachability> Create();

	[[nodiscard]] virtual rpl::producer<bool> availableValue() const = 0;
};

} // namespace base::Platform
