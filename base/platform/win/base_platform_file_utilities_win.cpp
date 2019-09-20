// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/win/base_platform_file_utilities_win.h"

#include "base/algorithm.h"

#include <QtCore/QString>
#include <QtCore/QDir>

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

} // namespace base::Platform
