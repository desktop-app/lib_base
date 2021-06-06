// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

namespace base::Platform {

struct BTDevice {
	std::optional<QString> mac;
	std::optional<QString> name;
	std::optional<uint> age;
	std::optional<int> rssi;
};

[[nodiscard]] std::vector<BTDevice> BTDevices();

} // namespace base::Platform
