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

inline constexpr bool IsWindows() {
	return true;
}

inline constexpr bool IsWindowsStoreBuild() {
#ifdef OS_WIN_STORE
	return true;
#else // OS_WIN_STORE
	return false;
#endif // OS_WIN_STORE
}

inline constexpr bool IsMac() { return false; }
inline constexpr bool IsOSXBuild() { return false; }
inline constexpr bool IsMacStoreBuild() { return false; }
inline bool IsMac10_6OrGreater() { return false; }
inline bool IsMac10_7OrGreater() { return false; }
inline bool IsMac10_8OrGreater() { return false; }
inline bool IsMac10_9OrGreater() { return false; }
inline bool IsMac10_10OrGreater() { return false; }
inline bool IsMac10_11OrGreater() { return false; }
inline bool IsMac10_12OrGreater() { return false; }
inline bool IsMac10_13OrGreater() { return false; }
inline bool IsMac10_14OrGreater() { return false; }
inline constexpr bool IsLinux() { return false; }
inline constexpr bool IsLinux32Bit() { return false; }
inline constexpr bool IsLinux64Bit() { return false; }
inline bool IsWayland() { return false; }
inline QString GetGlibCVersion() { return QString(); }

} // namespace Platform
