// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_url_scheme_linux.h"

#include "base/const_string.h"
#include "base/debug_log.h"

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
#include "base/platform/linux/base_linux_glibmm_helper.h"
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

#include <QtGui/QGuiApplication>
#include <QtWidgets/QWidget>

#include <kshell.h>
#include <ksandbox.h>

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
#include <glibmm.h>
#include <giomm.h>
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

namespace base::Platform {
namespace {

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
constexpr auto kSnapcraftSettingsService = "io.snapcraft.Settings"_cs;
constexpr auto kSnapcraftSettingsObjectPath = "/io/snapcraft/Settings"_cs;
constexpr auto kSnapcraftSettingsInterface = kSnapcraftSettingsService;

void SnapDefaultHandler(const QString &protocol) {
	try {
		const auto connection = Gio::DBus::Connection::get_sync(
			Gio::DBus::BusType::SESSION);

		auto reply = connection->call_sync(
			std::string(kSnapcraftSettingsObjectPath),
			std::string(kSnapcraftSettingsInterface),
			"GetSub",
			MakeGlibVariant(std::tuple{
				Glib::ustring("default-url-scheme-handler"),
				Glib::ustring(protocol.toStdString()),
			}),
			std::string(kSnapcraftSettingsService));

		const auto currentHandler = GlibVariantCast<Glib::ustring>(
			reply.get_child(0));

		const auto expectedHandler = qEnvironmentVariable("SNAP_NAME")
			+ ".desktop";

		if (currentHandler.c_str() == expectedHandler) {
			return;
		}

		const auto window = std::make_shared<QWidget>();
		window->setAttribute(Qt::WA_DontShowOnScreen);
		window->setWindowModality(Qt::ApplicationModal);
		window->show();

		connection->call(
			std::string(kSnapcraftSettingsObjectPath),
			std::string(kSnapcraftSettingsInterface),
			"SetSub",
			MakeGlibVariant(std::tuple{
				Glib::ustring("default-url-scheme-handler"),
				Glib::ustring(protocol.toStdString()),
				Glib::ustring(expectedHandler.toStdString()),
			}),
			[=](const Glib::RefPtr<Gio::AsyncResult> &result) {
				(void)window; // don't destroy until finish
				try {
					connection->call_finish(result);
				} catch (const std::exception &e) {
					LOG(("Snap Default Handler Error: %1")
						.arg(QString::fromStdString(e.what())));
				}
			},
			std::string(kSnapcraftSettingsService));
	} catch (const std::exception &e) {
		LOG(("Snap Default Handler Error: %1")
			.arg(QString::fromStdString(e.what())));
	}
}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

} // namespace

bool CheckUrlScheme(const UrlSchemeDescriptor &descriptor) {
#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
	const auto handlerType = "x-scheme-handler/"
		+ descriptor.protocol.toStdString();

	const auto neededCommandline = KShell::joinArgs(QStringList{
		descriptor.executable,
	} + KShell::splitArgs(descriptor.arguments) + QStringList{
		"--",
		"%u",
	}).toStdString();

	const auto currentAppInfo = Gio::AppInfo::get_default_for_type(
		handlerType,
		true);

	if (currentAppInfo) {
		return currentAppInfo->get_commandline() == neededCommandline;
	}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

	return false;
}

void RegisterUrlScheme(const UrlSchemeDescriptor &descriptor) {
#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
	if (KSandbox::isSnap()) {
		SnapDefaultHandler(descriptor.protocol);
		return;
	}

	if (CheckUrlScheme(descriptor)) {
		return;
	}
	UnregisterUrlScheme(descriptor);

	const auto handlerType = "x-scheme-handler/"
		+ descriptor.protocol.toStdString();

	const auto commandlineForCreator = KShell::joinArgs(QStringList{
		descriptor.executable,
	} + KShell::splitArgs(descriptor.arguments) + QStringList{
		"--",
	}).toStdString();

	const auto desktopId = QGuiApplication::desktopFileName().toStdString();
	if (!desktopId.empty()) {
		if (const auto appInfo = Gio::DesktopAppInfo::create(desktopId)) {
			if (appInfo->get_commandline() == commandlineForCreator + " %u") {
				appInfo->set_as_default_for_type(handlerType);
				return;
			}
		}
	}

	try {
		const auto newAppInfo = Gio::AppInfo::create_from_commandline(
			commandlineForCreator,
			descriptor.displayAppName.toStdString(),
			Gio::AppInfo::CreateFlags::SUPPORTS_URIS);

		if (newAppInfo) {
			newAppInfo->set_as_default_for_type(handlerType);
		}
	} catch (const std::exception &e) {
		LOG(("Register Url Scheme Error: %1").arg(
			QString::fromStdString(e.what())));
	}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION
}

void UnregisterUrlScheme(const UrlSchemeDescriptor &descriptor) {
#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
	const auto handlerType = "x-scheme-handler/"
		+ descriptor.protocol.toStdString();

	const auto neededCommandline = KShell::joinArgs(QStringList{
		descriptor.executable,
	} + KShell::splitArgs(descriptor.arguments) + QStringList{
		"--",
		"%u",
	}).toStdString();

	const auto registeredAppInfos = Gio::AppInfo::get_recommended_for_type(
		handlerType);

	for (const auto &appInfo : registeredAppInfos) {
		if (appInfo->get_commandline() == neededCommandline
			&& !appInfo->get_id().compare(0, 8, "userapp-")) {
			appInfo->do_delete();
		}
	}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION
}

} // namespace base::Platform
