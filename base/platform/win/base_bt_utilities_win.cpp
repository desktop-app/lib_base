// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/base_platform_bt_utilities.h"

#include "base/platform/win/base_windows_h.h"
#include "base/platform/win/base_windows_safe_library.h"

#include <bluetoothapis.h>

namespace base::Platform {
namespace {

HBLUETOOTH_DEVICE_FIND (__stdcall *BluetoothFindFirstDevice)(
	const BLUETOOTH_DEVICE_SEARCH_PARAMS *pbtsp,
	BLUETOOTH_DEVICE_INFO                *pbtdi);

BOOL (__stdcall *BluetoothFindNextDevice)(
	HBLUETOOTH_DEVICE_FIND hFind,
	BLUETOOTH_DEVICE_INFO  *pbtdi);

BOOL (__stdcall *BluetoothFindDeviceClose)(HBLUETOOTH_DEVICE_FIND hFind);

bool LoadBthprops() {
	static const auto Inited = [] {
		const auto lib = SafeLoadLibrary(u"bthprops.cpl"_q);
		if (!lib) return false;
		if (!LOAD_LIBRARY_SYMBOL(lib, BluetoothFindFirstDevice)) return false;
		if (!LOAD_LIBRARY_SYMBOL(lib, BluetoothFindNextDevice)) return false;
		if (!LOAD_LIBRARY_SYMBOL(lib, BluetoothFindDeviceClose)) return false;
		return true;
	}();

	return Inited;
}

BTDevice DeviceInfoToBTDevice(const BLUETOOTH_DEVICE_INFO &deviceInfo) {
	return {
		u"%1:%2:%3:%4:%5:%6"_q
			.arg(deviceInfo.Address.rgBytes[0], 2, 16, QLatin1Char('0'))
			.arg(deviceInfo.Address.rgBytes[1], 2, 16, QLatin1Char('0'))
			.arg(deviceInfo.Address.rgBytes[2], 2, 16, QLatin1Char('0'))
			.arg(deviceInfo.Address.rgBytes[3], 2, 16, QLatin1Char('0'))
			.arg(deviceInfo.Address.rgBytes[4], 2, 16, QLatin1Char('0'))
			.arg(deviceInfo.Address.rgBytes[5], 2, 16, QLatin1Char('0')),
		QString::fromWCharArray(deviceInfo.szName, wcslen(deviceInfo.szName)),
		std::nullopt, // TODO: convert stLastSeen from SYSTEMTIME to relative counter
		std::nullopt,
	};
}

} // namespace

std::vector<BTDevice> BTDevices() {
	std::vector<BTDevice> result;

	if (!LoadBthprops()) {
		return result;
	}

	BLUETOOTH_DEVICE_SEARCH_PARAMS searchParams{
		sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS),
		false,
		false,
		true,
		true,
		false,
		0,
		nullptr,
	};

	BLUETOOTH_DEVICE_INFO deviceInfo{sizeof(BLUETOOTH_DEVICE_INFO)};
	const auto hFind = BluetoothFindFirstDevice(&searchParams, &deviceInfo);
	if (!hFind) {
		return result;
	}

	const auto guard = gsl::finally([&] {
		BluetoothFindDeviceClose(hFind);
	});

	result.push_back(DeviceInfoToBTDevice(deviceInfo));
	
	while (BluetoothFindNextDevice(hFind, &deviceInfo)) {
		result.push_back(DeviceInfoToBTDevice(deviceInfo));
	}

	return result;
}

} // namespace base::Platform
