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
#include "base/platform/linux/base_linux_xdg_activation_token.h"
#include "base/algorithm.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtGui/QDesktopServices>

#include <giomm.h>
#include <gio/gio.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>

namespace base::Platform {
namespace {

void PortalShowInFolder(const QString &filepath, Fn<void()> fail) {
	const auto connection = [] {
		try {
			return Gio::DBus::Connection::get_sync(
				Gio::DBus::BusType::SESSION);
		} catch (...) {
			return Glib::RefPtr<Gio::DBus::Connection>();
		}
	}();

	if (!connection) {
		fail();
		return;
	}

	const auto fd = open(
		QFile::encodeName(filepath).constData(),
		O_RDONLY);

	if (fd == -1) {
		fail();
		return;
	}

	RunWithXdgActivationToken([=](const QString &activationToken) {
		connection->call(
			XDP::kObjectPath,
			"org.freedesktop.portal.OpenURI",
			"OpenDirectory",
			Glib::create_variant(std::tuple{
				XDP::ParentWindowID(),
				Glib::DBusHandle(),
				std::map<Glib::ustring, Glib::VariantBase>{
					{
						"activation_token",
						Glib::create_variant(
							Glib::ustring(activationToken.toStdString()))
					},
				},
			}),
			[=](const Glib::RefPtr<Gio::AsyncResult> &result) {
				try {
					connection->call_finish(result);
				} catch (...) {
					fail();
				}
				close(fd);
			},
			Gio::UnixFDList::create(std::vector<int>{ fd }),
			XDP::kService);
	});
}

void DBusShowInFolder(const QString &filepath, Fn<void()> fail) {
	const auto connection = [] {
		try {
			return Gio::DBus::Connection::get_sync(
				Gio::DBus::BusType::SESSION);
		} catch (...) {
			return Glib::RefPtr<Gio::DBus::Connection>();
		}
	}();

	if (!connection) {
		fail();
		return;
	}

	RunWithXdgActivationToken([=](const QString &startupId) {
		connection->call(
			"/org/freedesktop/FileManager1",
			"org.freedesktop.FileManager1",
			"ShowItems",
			Glib::create_variant(std::tuple{
				std::vector<Glib::ustring>{
					Glib::filename_to_uri(filepath.toStdString())
				},
				Glib::ustring(startupId.toStdString()),
			}),
			[=](const Glib::RefPtr<Gio::AsyncResult> &result) {
				try {
					connection->call_finish(result);
				} catch (...) {
					fail();
				}
			},
			"org.freedesktop.FileManager1");
	});
}

} // namespace

void ShowInFolder(const QString &filepath) {
	DBusShowInFolder(filepath, [=] {
		PortalShowInFolder(filepath, [=] {
			using namespace gi::repository;
			namespace Gio = gi::repository::Gio;
			if (Gio::AppInfo::launch_default_for_uri(
				GLib::filename_to_uri(
					GLib::path_get_dirname(filepath.toStdString()),
					nullptr),
				AppLaunchContext(),
				nullptr)) {
				return;
			}

			QDesktopServices::openUrl(
				QUrl::fromLocalFile(QFileInfo(filepath).absolutePath()));
		});
	});
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
