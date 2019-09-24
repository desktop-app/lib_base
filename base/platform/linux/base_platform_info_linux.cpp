// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_platform_info_linux.h"

#include <QtCore/QLocale>
#include <QtCore/QDate>

namespace Platform {

QString DeviceModelPretty() {
#ifdef Q_OS_LINUX64
	return "PC 64bit";
#else // Q_OS_LINUX64
	return "PC 32bit";
#endif // Q_OS_LINUX64
}

QString SystemVersionPretty() {
	const auto result = getenv("XDG_CURRENT_DESKTOP");
	const auto value = result ? QString::fromLatin1(result) : QString();
	const auto list = value.split(':', QString::SkipEmptyParts);
	return list.isEmpty() ? "Linux" : "Linux " + list[0];
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
	return QDate();
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

} // namespace Platform
