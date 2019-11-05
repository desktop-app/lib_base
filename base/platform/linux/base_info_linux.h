// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/platform/base_platform_info.h"

namespace Platform {

inline constexpr bool IsLinux() {
	return true;
}

inline constexpr bool IsLinux32Bit() {
#ifdef Q_OS_LINUX32
	return true;
#else // Q_OS_LINUX32
	return false;
#endif // Q_OS_LINUX32
}

inline constexpr bool IsLinux64Bit() {
#ifdef Q_OS_LINUX64
	return true;
#else // Q_OS_LINUX64
	return false;
#endif // Q_OS_LINUX64
}

inline constexpr bool IsWindows() { return false; }
inline constexpr bool IsWindowsStoreBuild() { return false; }
inline bool IsWindowsXPOrGreater() { return false; }
inline bool IsWindowsVistaOrGreater() { return false; }
inline bool IsWindows7OrGreater() { return false; }
inline bool IsWindows8OrGreater() { return false; }
inline bool IsWindows8Point1OrGreater() { return false; }
inline bool IsWindows10OrGreater() { return false; }
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

} // namespace Platform
