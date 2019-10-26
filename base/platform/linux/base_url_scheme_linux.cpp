// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_url_scheme_linux.h"

#include <QtCore/QDir>
#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>

namespace base::Platform {
namespace {

[[nodiscard]] QString GetHomeDir() {
	const auto result = QDir::homePath();

	if (result != QDir::rootPath()) {
		return result + '/';
	}

	struct passwd *pw = getpwuid(getuid());
	return (pw && pw->pw_dir && strlen(pw->pw_dir))
		? (QFile::decodeName(pw->pw_dir) + '/')
		: QString();
}

bool RunShellCommand(const QByteArray &command) {
	return (system(command.constData()) == 0);
}

[[nodiscard]] QByteArray EscapeShell(const QByteArray &content) {
	auto result = QByteArray();

	auto b = content.constData(), e = content.constEnd();
	for (auto ch = b; ch != e; ++ch) {
		if (*ch == ' ' || *ch == '"' || *ch == '\'' || *ch == '\\') {
			if (result.isEmpty()) {
				result.reserve(content.size() * 2);
			}
			if (ch > b) {
				result.append(b, ch - b);
			}
			result.append('\\');
			b = ch;
		}
	}
	if (result.isEmpty()) {
		return content;
	}

	if (e > b) {
		result.append(b, e - b);
	}
	return result;
}

bool RegisterDesktopFile(const UrlSchemeDescriptor &descriptor) {
	const auto home = GetHomeDir();
	if (home.isEmpty() || descriptor.executable.isEmpty() || descriptor.skipDesktopFileRegistration) {
		return false;
	}

	if (!QDir(home + ".local/").exists()) {
		return false;
	}
	const auto apps = home + ".local/share/applications/";
	const auto icons = home + ".local/share/icons/";
	if (!QDir(apps).exists()) {
		QDir().mkpath(apps);
	}
	if (!QDir(icons).exists()) {
		QDir().mkpath(icons);
	}

	const auto path = descriptor.desktopFileDir;
	const auto file = path + descriptor.desktopFileName + ".desktop";
	QDir().mkpath(path);
	auto f = QFile(file);
	if (!f.open(QIODevice::WriteOnly)) {
		return false;
	}
	const auto icon = icons + descriptor.iconFileName + ".png";
	if (descriptor.forceUpdateIcon) {
		QFile(icon).remove();
	}
	if (descriptor.forceUpdateIcon || !QFile(icon).exists()) {
		QFile(":/gui/art/logo.png").copy(icon);
	}

	auto s = QTextStream(&f);
	s.setCodec("UTF-8");
	s << "[Desktop Entry]\n";
	s << "Version=1.0\n";
	s << "Name=" << descriptor.displayAppName << "\n";
	s << "Comment=" << descriptor.displayAppDescription << "\n";
	s << "TryExec=" << EscapeShell(QFile::encodeName(descriptor.executable)) << "\n";
	s << "Exec=" << EscapeShell(QFile::encodeName(descriptor.executable)) << " -- %u\n";
	s << "Icon=" << descriptor.iconFileName << "\n";
	s << "Terminal=false\n";
	s << "StartupWMClass=" + QCoreApplication::instance()->applicationName() << "\n";
	s << "Type=Application\n";
	s << "MimeType=x-scheme-handler/" << descriptor.protocol << ";\n";
	s << "X-GNOME-UsesNotifications=true\n";
	f.close();

	if (!RunShellCommand("desktop-file-install --dir=" + EscapeShell(QFile::encodeName(home + ".local/share/applications")) + " --delete-original " + EscapeShell(QFile::encodeName(file)))) {
		return false;
	}
	RunShellCommand("update-desktop-database " + EscapeShell(QFile::encodeName(home + ".local/share/applications")));
	RunShellCommand("xdg-mime default telegramdesktop.desktop x-scheme-handler/tg");
	return true;
}

bool RegisterGnomeHandler(const UrlSchemeDescriptor &descriptor) {
	const auto protocolUtf = descriptor.protocol.toUtf8();
	if (!RunShellCommand("gconftool-2 -t string -s /desktop/gnome/url-handlers/" + protocolUtf + "/command " + EscapeShell(EscapeShell(QFile::encodeName(descriptor.executable)) + " -- %s"))) {
		return false;
	}
	RunShellCommand("gconftool-2 -t bool -s /desktop/gnome/url-handlers/" + protocolUtf + "/needs_terminal false");
	RunShellCommand("gconftool-2 -t bool -s /desktop/gnome/url-handlers/" + protocolUtf + "/enabled true");
	return true;
}

bool RegisterKdeHandler(const UrlSchemeDescriptor &descriptor) {
	const auto home = GetHomeDir();
	if (home.isEmpty() || descriptor.executable.isEmpty()) {
		return false;
	}

	const auto services = [&] {
		if (QDir(home + ".kde4/").exists()) {
			return home + ".kde4/share/kde4/services/";
		} else if (QDir(home + ".kde/").exists()) {
			return home + ".kde/share/kde4/services/";
		}
		return QString();
	}();
	if (services.isEmpty()) {
		return false;
	}
	if (!QDir(services).exists()) {
		QDir().mkpath(services);
	}

	const auto path = services;
	const auto file = path + descriptor.protocol + ".protocol";
	auto f = QFile(file);
	if (!f.open(QIODevice::WriteOnly)) {
		return false;
	}
	auto s = QTextStream(&f);
	s.setCodec("UTF-8");
	s << "[Protocol]\n";
	s << "exec=" << QFile::decodeName(EscapeShell(QFile::encodeName(descriptor.executable))) << " -- %u\n";
	s << "protocol=" << descriptor.protocol << "\n";
	s << "input=none\n";
	s << "output=none\n";
	s << "helper=true\n";
	s << "listing=false\n";
	s << "reading=false\n";
	s << "writing=false\n";
	s << "makedir=false\n";
	s << "deleting=false\n";
	f.close();
	return true;
}

} // namespace

bool CheckUrlScheme(const UrlSchemeDescriptor &descriptor) {
	return false;
}

void RegisterUrlScheme(const UrlSchemeDescriptor &descriptor) {
	RegisterDesktopFile(descriptor);
	RegisterGnomeHandler(descriptor);
	RegisterKdeHandler(descriptor);
}

} // namespace base::Platform
