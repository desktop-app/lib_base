// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_info_linux.h"

#include <QtCore/QJsonObject>
#include <QtCore/QLocale>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QVersionNumber>
#include <QtCore/QDate>

namespace Platform {
namespace {

void FallbackFontConfig(
		const QString &from,
		const QString &to,
		bool overwrite) {
	const auto finish = gsl::finally([&] {
		if (QFile(to).exists()) {
			qputenv("FONTCONFIG_FILE", QFile::encodeName(to));
		}
	});

	QProcess process;
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.start("fc-list", QStringList() << "--version");
	process.waitForFinished();
	if (process.exitCode() > 0) {
		return;
	}

	const auto result = QString::fromUtf8(process.readAllStandardOutput());

	const auto version = QVersionNumber::fromString(
		result.split("version ").last());
	if (version.isNull()) {
		return;
	}

	if (version < QVersionNumber::fromString("2.13")) {
		return;
	}

	if (!QFile(to).exists() || overwrite) {
		QFile(from).copy(to);
	}
}

} // namespace

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

void Start(QJsonObject options) {
	const auto from = options.value("custom_font_config_src").toString();
	const auto to = options.value("custom_font_config_dst").toString();
	if (!from.isEmpty() && !to.isEmpty()) {
		const auto keep = options.value("custom_font_config_keep").toInt();
		const auto overwrite = (keep != 1);
		FallbackFontConfig(from, to, overwrite);
	}
}

void Finish() {
}

} // namespace Platform
