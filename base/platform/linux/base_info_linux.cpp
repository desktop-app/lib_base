// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_info_linux.h"

#include <QtCore/QJsonObject>
#include <QtCore/QLocale>
#include <QtCore/QVersionNumber>
#include <QtCore/QDate>
#include <QtGui/QGuiApplication>
#include <gnu/libc-version.h>

namespace Platform {

QString DeviceModelPretty() {
	const auto cpuArch = QSysInfo::buildCpuArchitecture();

	if (cpuArch == qstr("x86_64")) {
		return "PC 64bit";
	} else if (cpuArch == qstr("i386")) {
		return "PC 32bit";
	}

	return "PC " + cpuArch;
}

QString SystemVersionPretty() {
	const auto result = getenv("XDG_CURRENT_DESKTOP");
	const auto value = result ? QString::fromLatin1(result) : QString();
	const auto list = value.split(':', QString::SkipEmptyParts);

	return "Linux "
		+ (list.isEmpty() ? QString() : list[0] + ' ')
		+ (IsWayland() ? "Wayland " : "X11 ")
		+ "glibc "
		+ GetGlibCVersion();
}

QString SystemCountry() {
	return QLocale::system().name().split('_').last();
}

QString SystemLanguage() {
	const auto system = QLocale::system();
	const auto languages = system.uiLanguages();
	return languages.isEmpty()
		? system.name().split('_').first()
		: languages.front();
}

QDate WhenSystemBecomesOutdated() {
	if (IsLinux32Bit()) {
		return QDate(2020, 9, 1);
	} else if (const auto version = GetGlibCVersion(); !version.isEmpty()) {
		if (QVersionNumber::fromString(version) < QVersionNumber(2, 23)) {
			return QDate(2020, 9, 1); // Older than Ubuntu 16.04.
		}
	}
	return QDate();
}

OutdateReason WhySystemBecomesOutdated() {
	return IsLinux32Bit() ? OutdateReason::Is32Bit : OutdateReason::IsOld;
}

int AutoUpdateVersion() {
	return 2;
}

QString AutoUpdateKey() {
	if (IsLinux32Bit()) {
		return "linux32";
	} else if (IsLinux64Bit()) {
		return "linux";
	} else {
		Unexpected("Platform in AutoUpdateKey.");
	}
}

QString GetGlibCVersion() {
	static const auto result = [&] {
		const auto version = QString::fromLatin1(gnu_get_libc_version());
		return QVersionNumber::fromString(version).isNull() ? QString() : version;
	}();
	return result;
}

bool IsWayland() {
	return QGuiApplication::platformName().startsWith("wayland", Qt::CaseInsensitive);
}

void Start(QJsonObject options) {
}

void Finish() {
}

} // namespace Platform
