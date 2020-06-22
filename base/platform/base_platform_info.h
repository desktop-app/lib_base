// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

class QJsonObject;
class QString;
class QDate;

namespace Platform {

enum class OutdateReason {
    IsOld,
    Is32Bit,
};

[[nodiscard]] QString DeviceModelPretty();
[[nodiscard]] QString SystemVersionPretty();
[[nodiscard]] QString SystemCountry();
[[nodiscard]] QString SystemLanguage();
[[nodiscard]] QDate WhenSystemBecomesOutdated();
[[nodiscard]] OutdateReason WhySystemBecomesOutdated();
[[nodiscard]] int AutoUpdateVersion();
[[nodiscard]] QString AutoUpdateKey();

[[nodiscard]] constexpr bool IsWindows();
[[nodiscard]] constexpr bool IsWindowsStoreBuild();
[[nodiscard]] bool IsWindowsXPOrGreater();
[[nodiscard]] bool IsWindowsVistaOrGreater();
[[nodiscard]] bool IsWindows7OrGreater();
[[nodiscard]] bool IsWindows8OrGreater();
[[nodiscard]] bool IsWindows8Point1OrGreater();
[[nodiscard]] bool IsWindows10OrGreater();

[[nodiscard]] constexpr bool IsMac();
[[nodiscard]] constexpr bool IsOSXBuild();
[[nodiscard]] constexpr bool IsMacStoreBuild();
[[nodiscard]] bool IsMac10_6OrGreater();
[[nodiscard]] bool IsMac10_7OrGreater();
[[nodiscard]] bool IsMac10_8OrGreater();
[[nodiscard]] bool IsMac10_9OrGreater();
[[nodiscard]] bool IsMac10_10OrGreater();
[[nodiscard]] bool IsMac10_11OrGreater();
[[nodiscard]] bool IsMac10_12OrGreater();
[[nodiscard]] bool IsMac10_13OrGreater();
[[nodiscard]] bool IsMac10_14OrGreater();

[[nodiscard]] constexpr bool IsLinux();
[[nodiscard]] constexpr bool IsLinux32Bit();
[[nodiscard]] constexpr bool IsLinux64Bit();
[[nodiscard]] bool IsWayland();
[[nodiscard]] QString GetGlibCVersion();

void Start(QJsonObject settings);
void Finish();

} // namespace Platform

#ifdef Q_OS_MAC
#include "base/platform/mac/base_info_mac.h"
#elif defined Q_OS_UNIX // Q_OS_MAC
#include "base/platform/linux/base_info_linux.h"
#elif defined Q_OS_WIN // Q_OS_MAC || Q_OS_UNIX
#include "base/platform/win/base_info_win.h"
#endif // Q_OS_MAC || Q_OS_UNIX || Q_OS_WIN
