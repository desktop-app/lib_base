// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_file_utilities_linux.h"

#include "base/platform/base_platform_file_utilities.h"
#include "base/platform/linux/base_linux_xdp_utilities.h"
#include "base/platform/linux/base_linux_app_launch_context.h"
#include "base/platform/linux/base_linux_wayland_integration.h"
#include "base/algorithm.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtGui/QDesktopServices>

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

bool PortalShowInFolder(const QString &filepath) {
	try {
		const auto connection = Gio::DBus::Connection::get_sync(
			Gio::DBus::BusType::SESSION);

		const auto fd = open(
			QFile::encodeName(filepath).constData(),
			O_RDONLY);

		if (fd == -1) {
			return false;
		}

		const auto guard = gsl::finally([&] { close(fd); });

		const auto activationToken = []() -> Glib::ustring {
			if (const auto integration = WaylandIntegration::Instance()) {
				if (const auto token = integration->activationToken()
					; !token.isNull()) {
					return token.toStdString();
				}
			}
			return {};
		}();

		auto outFdList = Glib::RefPtr<Gio::UnixFDList>();

		connection->call_sync(
			XDP::kObjectPath,
			"org.freedesktop.portal.OpenURI",
			"OpenDirectory",
			Glib::create_variant(std::tuple{
				Glib::ustring(),
				Glib::DBusHandle(),
				std::map<Glib::ustring, Glib::VariantBase>{
					{
						"activation_token",
						Glib::create_variant(activationToken)
					},
				},
			}),
			Gio::UnixFDList::create(std::vector<int>{ fd }),
			outFdList,
			XDP::kService);

		return true;
	} catch (...) {
	}

	return false;
}

bool DBusShowInFolder(const QString &filepath) {
	try {
		const auto connection = Gio::DBus::Connection::get_sync(
			Gio::DBus::BusType::SESSION);

		const auto startupId = []() -> Glib::ustring {
			if (const auto integration = WaylandIntegration::Instance()) {
				return integration->activationToken().toStdString();
			}
			return {};
		}();

		connection->call_sync(
			"/org/freedesktop/FileManager1",
			"org.freedesktop.FileManager1",
			"ShowItems",
			Glib::create_variant(std::tuple{
				std::vector<Glib::ustring>{
					Glib::filename_to_uri(filepath.toStdString())
				},
				startupId,
			}),
			"org.freedesktop.FileManager1");

		return true;
	} catch (...) {
	}

	return false;
}

} // namespace

bool ShowInFolder(const QString &filepath) {
	if (DBusShowInFolder(filepath)) {
		return true;
	}

	if (PortalShowInFolder(filepath)) {
		return true;
	}

	try {
		if (Gio::AppInfo::launch_default_for_uri(
			Glib::filename_to_uri(
				Glib::path_get_dirname(filepath.toStdString())),
			AppLaunchContext())) {
			return true;
		}
	} catch (...) {
	}

	return QDesktopServices::openUrl(
		QUrl::fromLocalFile(QFileInfo(filepath).absolutePath()));
}

QString CurrentExecutablePath(int argc, char *argv[]) {
	const auto exeLink = QFileInfo(u"/proc/%1/exe"_q.arg(getpid()));
	if (exeLink.exists() && exeLink.isSymLink()) {
		return exeLink.canonicalFilePath();
	}

	// Fallback to the first command line argument.
	if (argc) {
		const auto argv0 = QFile::decodeName(argv[0]);
		if (!argv0.isEmpty() && !argv0.contains(QLatin1Char('/'))) {
			const auto argv0InPath = QStandardPaths::findExecutable(argv0);
			if (!argv0InPath.isEmpty()) {
				return argv0InPath;
			}
		}
		return QFileInfo(argv0).absoluteFilePath();
	}

	return QString();
}

void RemoveQuarantine(const QString &path) {
}

QString BundledResourcesPath() {
	Unexpected("BundledResourcesPath not implemented.");
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
