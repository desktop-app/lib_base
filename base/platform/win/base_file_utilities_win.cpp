// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/win/base_file_utilities_win.h"

#include "base/algorithm.h"

#include <QtCore/QString>
#include <QtCore/QDir>

#include <array>
#include <string>
#include <Shlwapi.h>
#include <shlobj.h>

namespace base::Platform {

bool ShowInFolder(const QString &filepath) {
	auto nativePath = QDir::toNativeSeparators(filepath);
	const auto path = nativePath.toStdWString();
	if (const auto pidl = ILCreateFromPathW(path.c_str())) {
		const auto result = SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);
		ILFree(pidl);
		return (result == S_OK);
	}
	const auto pathEscaped = nativePath.replace('"', QString("\"\""));
	const auto command = ("/select," + pathEscaped).toStdWString();
	const auto result = int(ShellExecute(
		0,
		0,
		L"explorer",
		command.c_str(),
		0,
		SW_SHOWNORMAL));
	return (result > 32);
}

QString FileNameFromUserString(QString name) {
	const auto kBadExtensions = { qstr(".lnk"), qstr(".scf") };
	const auto kMaskExtension = qstr(".download");
	for (const auto extension : kBadExtensions) {
		if (name.endsWith(extension, Qt::CaseInsensitive)) {
			name += kMaskExtension;
		}
	}

	static const auto BadNames = {
		qstr("CON"),
		qstr("PRN"),
		qstr("AUX"),
		qstr("NUL"),
		qstr("COM1"),
		qstr("COM2"),
		qstr("COM3"),
		qstr("COM4"),
		qstr("COM5"),
		qstr("COM6"),
		qstr("COM7"),
		qstr("COM8"),
		qstr("COM9"),
		qstr("LPT1"),
		qstr("LPT2"),
		qstr("LPT3"),
		qstr("LPT4"),
		qstr("LPT5"),
		qstr("LPT6"),
		qstr("LPT7"),
		qstr("LPT8"),
		qstr("LPT9")
	};
	for (const auto bad : BadNames) {
		if (name.startsWith(bad, Qt::CaseInsensitive)) {
			if (name.size() == bad.size() || name[bad.size()] == '.') {
				name = '_' + name;
				break;
			}
		}
	}
	return name;
}

QString CurrentExecutablePath(int argc, char *argv[]) {
	auto result = std::array<WCHAR, MAX_PATH + 1>{ 0 };
	const auto count = GetModuleFileName(
		nullptr,
		result.data(),
		MAX_PATH + 1);
	if (count < MAX_PATH + 1) {
		const auto info = QFileInfo(QDir::fromNativeSeparators(
			QString::fromWCharArray(result.data(), count)));
		return info.absoluteFilePath();
	}

	// Fallback to the first command line argument.
	auto argsCount = 0;
	if (const auto args = CommandLineToArgvW(GetCommandLine(), &argsCount)) {
		auto info = QFileInfo(QDir::fromNativeSeparators(
			QString::fromWCharArray(args[0])));
		LocalFree(args);
		return info.absoluteFilePath();
	}
	return QString();
}

} // namespace base::Platform
