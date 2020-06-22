// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/platform/base_platform_info.h"

namespace Platform {

inline OutdateReason WhySystemBecomesOutdated() {
	return OutdateReason::IsOld;
}

inline constexpr bool IsMac() {
	return true;
}

inline constexpr bool IsOSXBuild() {
#ifdef OS_OSX
	return true;
#else // OS_OSX
	return false;
#endif // OS_OSX
}

inline constexpr bool IsMacStoreBuild() {
#ifdef OS_MAC_STORE
	return true;
#else // OS_MAC_STORE
	return false;
#endif // OS_MAC_STORE
}

inline constexpr bool IsWindows() { return false; }
inline constexpr bool IsWindowsStoreBuild() { return false; }
inline bool IsWindowsXPOrGreater() { return false; }
inline bool IsWindowsVistaOrGreater() { return false; }
inline bool IsWindows7OrGreater() { return false; }
inline bool IsWindows8OrGreater() { return false; }
inline bool IsWindows8Point1OrGreater() { return false; }
inline bool IsWindows10OrGreater() { return false; }
inline constexpr bool IsLinux() { return false; }
inline constexpr bool IsLinux32Bit() { return false; }
inline constexpr bool IsLinux64Bit() { return false; }
inline bool IsWayland() { return false; }
inline QString GetGlibCVersion() { return QString(); }

} // namespace Platform
