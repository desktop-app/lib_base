// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/base_platform_wifi_utilities.h"

#include "base/platform/win/base_windows_h.h"
#include "base/platform/win/base_windows_safe_library.h"

#include <wlanapi.h>

namespace base::Platform {
namespace {

DWORD (__stdcall *WlanOpenHandle)(
	DWORD   dwClientVersion,
	PVOID   pReserved,
	PDWORD  pdwNegotiatedVersion,
	PHANDLE phClientHandle);

DWORD (__stdcall *WlanCloseHandle)(
	HANDLE hClientHandle,
	PVOID  pReserved);

void (__stdcall *WlanFreeMemory)(PVOID pMemory);

DWORD (__stdcall *WlanEnumInterfaces)(
	HANDLE                    hClientHandle,
	PVOID                     pReserved,
	PWLAN_INTERFACE_INFO_LIST *ppInterfaceList);

DWORD (__stdcall *WlanGetNetworkBssList)(
	HANDLE            hClientHandle,
	const GUID        *pInterfaceGuid,
	const PDOT11_SSID pDot11Ssid,
	DOT11_BSS_TYPE    dot11BssType,
	BOOL              bSecurityEnabled,
	PVOID             pReserved,
	PWLAN_BSS_LIST    *ppWlanBssList);

bool LoadWlanapi() {
	static const auto Inited = [] {
		const auto lib = SafeLoadLibrary(u"wlanapi.dll"_q);
		if (!lib) return false;
		if (!LOAD_LIBRARY_SYMBOL(lib, WlanOpenHandle)) return false;
		if (!LOAD_LIBRARY_SYMBOL(lib, WlanCloseHandle)) return false;
		if (!LOAD_LIBRARY_SYMBOL(lib, WlanFreeMemory)) return false;
		if (!LOAD_LIBRARY_SYMBOL(lib, WlanEnumInterfaces)) return false;
		if (!LOAD_LIBRARY_SYMBOL(lib, WlanGetNetworkBssList)) return false;
		return true;
	}();

	return Inited;
}

} // namespace

std::vector<WiFiNetwork> WiFiNetworks() {
	std::vector<WiFiNetwork> result;

	if (!LoadWlanapi()) {
		return result;
	}

	HANDLE hClient = nullptr;
	DWORD wlanApiVersion = 0;
	if (WlanOpenHandle(
		2,
		nullptr,
		&wlanApiVersion,
		&hClient) != ERROR_SUCCESS) {
		return result;
	}

	const auto hClientGuard = gsl::finally([&] {
		WlanCloseHandle(hClient, nullptr);
	});

	PWLAN_INTERFACE_INFO_LIST pIfList = nullptr;
	if (WlanEnumInterfaces(hClient, nullptr, &pIfList) != ERROR_SUCCESS) {
		return result;
	}

	const auto pIfListGuard = gsl::finally([&] {
		WlanFreeMemory(pIfList);
	});

	std::vector<WLAN_INTERFACE_INFO> interfaceList(
		pIfList->InterfaceInfo,
		pIfList->InterfaceInfo + pIfList->dwNumberOfItems);
	
	for (const auto &interfaceInfo : interfaceList) {
		PWLAN_BSS_LIST pBssList = nullptr;
		if (WlanGetNetworkBssList(
			hClient,
			&interfaceInfo.InterfaceGuid,
			nullptr,
			dot11_BSS_type_any,
			false,
			nullptr,
			&pBssList) != ERROR_SUCCESS) {
			continue;
		}

		const auto pBssListGuard = gsl::finally([&] {
			WlanFreeMemory(pBssList);
		});

		std::vector<WLAN_BSS_ENTRY> bssList(
			pBssList->wlanBssEntries,
			pBssList->wlanBssEntries + pBssList->dwNumberOfItems);

		for (const auto &bssEntry : bssList) {
			std::vector<char> ssid(
				bssEntry.dot11Ssid.ucSSID,
				bssEntry.dot11Ssid.ucSSID + bssEntry.dot11Ssid.uSSIDLength);

			result.push_back({
				u"%1:%2:%3:%4:%5:%6"_q
					.arg(bssEntry.dot11Bssid[0], 2, 16, QLatin1Char('0'))
					.arg(bssEntry.dot11Bssid[1], 2, 16, QLatin1Char('0'))
					.arg(bssEntry.dot11Bssid[2], 2, 16, QLatin1Char('0'))
					.arg(bssEntry.dot11Bssid[3], 2, 16, QLatin1Char('0'))
					.arg(bssEntry.dot11Bssid[4], 2, 16, QLatin1Char('0'))
					.arg(bssEntry.dot11Bssid[5], 2, 16, QLatin1Char('0')),
				!ssid.empty()
					? std::make_optional(QByteArray(ssid.data(), ssid.size()))
					: std::nullopt,
				std::nullopt,
				std::nullopt,
				std::nullopt,
				bssEntry.lRssi,
				std::nullopt,
			});
		}
	}

	return result;
}

} // namespace base::Platform
