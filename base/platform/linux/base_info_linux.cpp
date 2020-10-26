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

// this file is used on both Linux & BSD
#ifdef Q_OS_LINUX
#include <gnu/libc-version.h>
#endif // Q_OS_LINUX

namespace Platform {
namespace {

QString GetDesktopEnvironment() {
	const auto value = qgetenv("XDG_CURRENT_DESKTOP");
	return value.contains(':')
		? value.left(value.indexOf(':'))
		: value;
}

} // namespace

QString DeviceModelPretty() {
#ifdef Q_PROCESSOR_X86_64
	return "PC 64bit";
#elif defined Q_PROCESSOR_X86_32 // Q_PROCESSOR_X86_64
	return "PC 32bit";
#else // Q_PROCESSOR_X86_64 || Q_PROCESSOR_X86_32
	return "PC " + QSysInfo::buildCpuArchitecture();
#endif // else for Q_PROCESSOR_X86_64 || Q_PROCESSOR_X86_32
}

QString SystemVersionPretty() {
	static const auto result = [&] {
		QStringList resultList{};

#ifdef Q_OS_LINUX
		resultList << "Linux";
#else // Q_OS_LINUX
		resultList << QSysInfo::kernelType();
#endif // !Q_OS_LINUX

		const auto desktopEnvironment = GetDesktopEnvironment();
		if (!desktopEnvironment.isEmpty()) {
			resultList << desktopEnvironment;
		}

		if (IsWayland()) {
			resultList << "Wayland";
		} else {
			resultList << "X11";
		}

		const auto libcName = GetLibcName();
		const auto libcVersion = GetLibcVersion();
		if (!libcVersion.isEmpty()) {
			if (!libcName.isEmpty()) {
				resultList << libcName;
			} else {
				resultList << "libc";
			}
			resultList << libcVersion;
		}

		return resultList.join(' ');
	}();

	return result;
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
	const auto libcName = GetLibcName();
	const auto libcVersion = GetLibcVersion();

	if (IsLinux32Bit()) {
		return QDate(2020, 9, 1);
	} else if (libcName == qstr("glibc") && !libcVersion.isEmpty()) {
		if (QVersionNumber::fromString(libcVersion) < QVersionNumber(2, 23)) {
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

QString GetLibcName() {
#ifdef Q_OS_LINUX
	return "glibc";
#endif // Q_OS_LINUX

	return QString();
}

QString GetLibcVersion() {
#ifdef Q_OS_LINUX
	static const auto result = [&] {
		const auto version = QString::fromLatin1(gnu_get_libc_version());
		return QVersionNumber::fromString(version).isNull() ? QString() : version;
	}();
	return result;
#endif // Q_OS_LINUX

	return QString();
}

bool IsWayland() {
	return QGuiApplication::platformName().startsWith("wayland", Qt::CaseInsensitive);
}

void Start(QJsonObject options) {
}

void Finish() {
}

} // namespace Platform
