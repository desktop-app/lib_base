// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_file_utilities_linux.h"

#include "base/platform/base_platform_file_utilities.h"
#include "base/platform/linux/base_linux_xdp_utilities.h"
#include "base/platform/linux/base_linux_xdg_activation_token.h"
#include "base/algorithm.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtGui/QDesktopServices>

#include <xdpopenuri/xdpopenuri.hpp>
#include <xdgfilemanager1/xdgfilemanager1.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>

namespace base::Platform {
namespace {

using namespace gi::repository;

void PortalShowInFolder(const QString &filepath, Fn<void()> fail) {
	XdpOpenURI::OpenURIProxy::new_for_bus(
		Gio::BusType::SESSION_,
		Gio::DBusProxyFlags::NONE_,
		XDP::kService,
		XDP::kObjectPath,
		[=](GObject::Object, Gio::AsyncResult res) {
			auto interface = XdpOpenURI::OpenURI(
				XdpOpenURI::OpenURIProxy::new_for_bus_finish(res, nullptr));

			if (!interface) {
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

			RunWithXdgActivationToken([=](
					const QString &activationToken) mutable {
				interface.call_open_directory(
					std::string(XDP::ParentWindowID()),
					GLib::Variant::new_handle(0),
					GLib::Variant::new_array({
						GLib::Variant::new_dict_entry(
							GLib::Variant::new_string("activation_token"),
							GLib::Variant::new_variant(
								GLib::Variant::new_string(
									activationToken.toStdString()))),
					}),
					Gio::UnixFDList::new_from_array((std::array{
						fd,
					}).data(), 1),
					{},
					[=](GObject::Object, Gio::AsyncResult res) mutable {
						if (!interface.call_open_directory_finish(res)) {
							fail();
						}
						close(fd);
					});
			});
		});
}

void DBusShowInFolder(const QString &filepath, Fn<void()> fail) {
	XdgFileManager1::FileManager1Proxy::new_for_bus(
		Gio::BusType::SESSION_,
		Gio::DBusProxyFlags::NONE_,
		"org.freedesktop.FileManager1",
		"/org/freedesktop/FileManager1",
		[=](GObject::Object, Gio::AsyncResult res) {
			auto interface = XdgFileManager1::FileManager1(
				XdgFileManager1::FileManager1Proxy::new_for_bus_finish(
					res,
					nullptr));

			if (!interface) {
				fail();
				return;
			}

			RunWithXdgActivationToken([=](const QString &startupId) mutable {
				const auto callbackWrap = gi::unwrap(
					Gio::AsyncReadyCallback(
						[=](GObject::Object, Gio::AsyncResult res) mutable {
							if (!interface.call_show_items_finish(res)) {
								fail();
							}
						}
					),
					gi::scope_async);

				xdg_file_manager1_file_manager1_call_show_items(
					interface.gobj_(),
					(std::array<const char*, 2>{
						GLib::filename_to_uri(
							filepath.toStdString(),
							nullptr
						).c_str(),
						nullptr,
					}).data(),
					startupId.toStdString().c_str(),
					nullptr,
					&callbackWrap->wrapper,
					callbackWrap);
			});
		});
}

} // namespace

void ShowInFolder(const QString &filepath) {
	DBusShowInFolder(filepath, [=] {
		PortalShowInFolder(filepath, [=] {
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

QString FileNameFromUserString(QString name) {
	return name;
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
