// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

namespace base::Platform {

struct CellNetwork {
	enum class Type {
		Unknown,
		GSM_2G,
		GSM_3G,
		GSM_4G,
	};
	std::optional<QString> carrier;
	std::optional<Type> type;
	std::optional<uint> mcc;
	std::optional<uint> mnc;
	std::optional<uint> cellid;
	std::optional<uint> lac;
	std::optional<uint> psc;
	std::optional<int> rssi;
	std::optional<uint> ta;
	std::optional<uint> age;
};

[[nodiscard]] std::vector<CellNetwork> CellNetworks();

} // namespace base::Platform
