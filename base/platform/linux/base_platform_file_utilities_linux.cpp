// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_platform_file_utilities_linux.h"

#include "base/platform/base_platform_file_utilities.h"
#include "base/algorithm.h"

#include <QtCore/QProcess>
#include <QtCore/QFileInfo>

namespace base::Platform {

bool ShowInFolder(const QString &filepath) {
	const auto absolutePath = QFileInfo(filepath).absoluteFilePath();
	QProcess process;
	process.start(
		"xdg-mime",
		QStringList() << "query" << "default" << "inode/directory");
	process.waitForFinished();
	auto output = QString::fromLatin1(process.readLine().simplified());
	auto command = QString("xdg-open");
	auto arguments = QStringList();
	if (output == qstr("dolphin.desktop")
		|| output == qstr("org.kde.dolphin.desktop")) {
		command = "dolphin";
		arguments << "--select" << absolutePath;
	} else if (output == qstr("nautilus.desktop")
		|| output == qstr("org.gnome.Nautilus.desktop")
		|| output == qstr("nautilus-folder-handler.desktop")) {
		command = "nautilus";
		arguments << absolutePath;
	} else if (output == qstr("nemo.desktop")) {
		command = "nemo";
		arguments << "--no-desktop" << absolutePath;
	} else if (output == qstr("konqueror.desktop")
		|| output == qstr("kfmclient_dir.desktop")) {
		command = "konqueror";
		arguments << "--select" << absolutePath;
	} else {
		arguments << QFileInfo(filepath).absoluteDir().absolutePath();
	}
	return process.startDetached(command, arguments);
}

} // namespace base::Platform