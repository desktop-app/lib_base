// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include <vector>

class QJsonObject;
class QString;
class QDate;

namespace base::Platform {

[[nodiscard]] bool IsDeviceModelOk(const QString &model);

[[nodiscard]] QString SimplifyDeviceModel(QString model);
[[nodiscard]] QString SimplifyGoodDeviceModel(
	QString model,
	std::vector<QString> remove);
[[nodiscard]] QString ProductNameToDeviceModel(const QString &productName);
[[nodiscard]] QString FinalizeDeviceModel(QString model);

} // namespace base::Platform

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
[[nodiscard]] constexpr bool IsWindows32Bit();
[[nodiscard]] constexpr bool IsWindows64Bit();
[[nodiscard]] constexpr bool IsWindowsStoreBuild();
[[nodiscard]] bool IsWindows7OrGreater();
[[nodiscard]] bool IsWindows8OrGreater();
[[nodiscard]] bool IsWindows8Point1OrGreater();
[[nodiscard]] bool IsWindows10OrGreater();
[[nodiscard]] bool IsWindows11OrGreater();

[[nodiscard]] constexpr bool IsMac();
[[nodiscard]] constexpr bool IsMacStoreBuild();
[[nodiscard]] bool IsMac10_12OrGreater();
[[nodiscard]] bool IsMac10_13OrGreater();
[[nodiscard]] bool IsMac10_14OrGreater();
[[nodiscard]] bool IsMac10_15OrGreater();
[[nodiscard]] bool IsMac11_0OrGreater();

[[nodiscard]] constexpr bool IsLinux();
[[nodiscard]] bool IsX11();
[[nodiscard]] bool IsWayland();
[[nodiscard]] bool IsXwayland();

[[nodiscard]] QString GetLibcName();
[[nodiscard]] QString GetLibcVersion();
[[nodiscard]] QString GetWindowManager();

void Start(QJsonObject settings);
void Finish();

} // namespace Platform

#ifdef Q_OS_WIN
#include "base/platform/win/base_info_win.h"
#elif defined Q_OS_MAC // Q_OS_WIN
#include "base/platform/mac/base_info_mac.h"
#else // Q_OS_WIN || Q_OS_MAC
#include "base/platform/linux/base_info_linux.h"
#endif // else for Q_OS_WIN || Q_OS_MAC
