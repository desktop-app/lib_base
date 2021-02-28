// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_file_utilities_linux.h"

#include "base/platform/base_platform_file_utilities.h"
#include "base/algorithm.h"

#include <QtCore/QProcess>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtGui/QDesktopServices>

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusUnixFileDescriptor>
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

#undef foreach
#undef slots
#undef signals
#undef emit
#include <glibmm.h>
#include <giomm.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>

namespace base::Platform {
namespace {

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
bool PortalShowInFolder(const QString &filepath) {
	const auto fd = open(QFile::encodeName(filepath).constData(), O_RDONLY);
	if (fd == -1) {
		return false;
	}

	auto message = QDBusMessage::createMethodCall(
		"org.freedesktop.portal.Desktop",
		"/org/freedesktop/portal/desktop",
		"org.freedesktop.portal.OpenURI",
		"OpenDirectory");

	message.setArguments({
		QString(),
		QVariant::fromValue(QDBusUnixFileDescriptor(fd)),
		QVariantMap()
	});

	close(fd);

	const QDBusError error = QDBusConnection::sessionBus().call(message);
	return !error.isValid();
}

bool DBusShowInFolder(const QString &filepath) {
	auto message = QDBusMessage::createMethodCall(
		"org.freedesktop.FileManager1",
		"/org/freedesktop/FileManager1",
		"org.freedesktop.FileManager1",
		"ShowItems");

	message.setArguments({
		QStringList{QUrl::fromLocalFile(filepath).toString()},
		QString()
	});

	const QDBusError error = QDBusConnection::sessionBus().call(message);
	return !error.isValid();
}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

bool ProcessShowInFolder(const QString &filepath) {
	const auto fileManager = [] {
		try {
			const auto appInfo = Gio::AppInfo::get_default_for_type(
				"inode/directory",
				false);

			if (appInfo) {
				return QString::fromStdString(appInfo->get_id());
			}
		} catch (...) {
		}

		return QString();
	}();

	if (fileManager == qstr("dolphin.desktop")
		|| fileManager == qstr("org.kde.dolphin.desktop")) {
		return QProcess::startDetached("dolphin", {
			"--select",
			filepath
		});
	} else if (fileManager == qstr("nautilus.desktop")
		|| fileManager == qstr("org.gnome.Nautilus.desktop")
		|| fileManager == qstr("nautilus-folder-handler.desktop")) {
		return QProcess::startDetached("nautilus", {
			filepath
		});
	} else if (fileManager == qstr("nemo.desktop")) {
		return QProcess::startDetached("nemo", {
			"--no-desktop",
			filepath
		});
	} else if (fileManager == qstr("konqueror.desktop")
		|| fileManager == qstr("kfmclient_dir.desktop")) {
		return QProcess::startDetached("konqueror", {
			"--select",
			filepath
		});
	}

	return false;
}

} // namespace

bool ShowInFolder(const QString &filepath) {
	const auto absolutePath = QFileInfo(filepath).absoluteFilePath();
	const auto absoluteDirPath = QFileInfo(filepath)
		.absoluteDir()
		.absolutePath();

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
	if (DBusShowInFolder(absolutePath)) {
		return true;
	}

	if (PortalShowInFolder(absolutePath)) {
		return true;
	}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

	if (ProcessShowInFolder(absolutePath)) {
		return true;
	}

	try {
		if (Gio::AppInfo::launch_default_for_uri(
			Glib::filename_to_uri(absoluteDirPath.toStdString()))) {
			return true;
		}
	} catch (...) {
	}

	if (QDesktopServices::openUrl(QUrl::fromLocalFile(absoluteDirPath))) {
		return true;
	}

	return false;
}

QString CurrentExecutablePath(int argc, char *argv[]) {
	constexpr auto kMaxPath = 1024;
	char result[kMaxPath] = { 0 };
	auto count = readlink("/proc/self/exe", result, kMaxPath);
	if (count > 0) {
		auto filename = QFile::decodeName(result);
		auto deletedPostfix = qstr(" (deleted)");
		if (filename.endsWith(deletedPostfix)
			&& !QFileInfo(filename).exists()) {
			filename.chop(deletedPostfix.size());
		}
		return filename;
	}

	// Fallback to the first command line argument.
	return argc ? QFile::decodeName(argv[0]) : QString();
}

void RemoveQuarantine(const QString &path) {
}

// From http://stackoverflow.com/questions/2256945/removing-a-non-empty-directory-programmatically-in-c-or-c
bool DeleteDirectory(QString path) {
	if (path.endsWith('/')) {
		path.chop(1);
	}
	const auto pathRaw = QFile::encodeName(path);
	const auto d = opendir(pathRaw.constData());
	if (!d) {
		return false;
	}

	while (struct dirent *p = readdir(d)) {
		// Skip the names "." and ".." as we don't want to recurse on them.
		if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
			continue;
		}

		const auto fname = path + '/' + p->d_name;
		const auto encoded = QFile::encodeName(fname);
		struct stat statbuf;
		if (!stat(encoded.constData(), &statbuf)) {
			if (S_ISDIR(statbuf.st_mode)) {
				if (!DeleteDirectory(fname)) {
					closedir(d);
					return false;
				}
			} else {
				if (unlink(encoded.constData())) {
					closedir(d);
					return false;
				}
			}
		}
	}
	closedir(d);

	return !rmdir(pathRaw.constData());
}

bool RenameWithOverwrite(const QString &from, const QString &to) {
	const auto fromPath = QFile::encodeName(from);
	const auto toPath = QFile::encodeName(to);
	return (rename(fromPath.constData(), toPath.constData()) == 0);
}

void FlushFileData(QFile &file) {
	file.flush();
	if (const auto descriptor = file.handle()) {
		fsync(descriptor);
	}
}

} // namespace base::Platform
