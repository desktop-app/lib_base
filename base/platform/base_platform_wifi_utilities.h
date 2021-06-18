// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

namespace base::Platform {

struct WiFiNetwork {
	std::optional<QString> bssid;
	std::optional<QByteArray> ssid;
	std::optional<uint> age;
	std::optional<uint> channel;
	std::optional<uint> frequency;
	std::optional<int> rssi;
	std::optional<int> snr;
};

[[nodiscard]] std::vector<WiFiNetwork> WiFiNetworks();

} // namespace base::Platform
