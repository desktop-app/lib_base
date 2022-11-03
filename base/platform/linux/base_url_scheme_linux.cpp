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

#include <QtCore/QProcess>
#include <QtWidgets/QWidget>

#include <kshell.h>
#include <ksandbox.h>

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
#include <gio/gio.h>
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

		const auto loop = Glib::MainLoop::create();

		connection->call(
			std::string(kSnapcraftSettingsObjectPath),
			std::string(kSnapcraftSettingsInterface),
			"SetSub",
			MakeGlibVariant(std::tuple{
				Glib::ustring("default-url-scheme-handler"),
				Glib::ustring(protocol.toStdString()),
				Glib::ustring(expectedHandler.toStdString()),
			}),
			[&](const Glib::RefPtr<Gio::AsyncResult> &result) {
				try {
					connection->call_finish(result);
				} catch (const std::exception &e) {
					LOG(("Snap Default Handler Error: %1")
						.arg(QString::fromStdString(e.what())));
				}

				loop->quit();
			},
			std::string(kSnapcraftSettingsService));

		QWidget window;
		window.setAttribute(Qt::WA_DontShowOnScreen);
		window.setWindowModality(Qt::ApplicationModal);
		window.show();
		loop->run();
	} catch (const std::exception &e) {
		LOG(("Snap Default Handler Error: %1")
			.arg(QString::fromStdString(e.what())));
	}
}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

} // namespace

bool CheckUrlScheme(const UrlSchemeDescriptor &descriptor) {
#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
	const auto handlerType = QString("x-scheme-handler/%1")
		.arg(descriptor.protocol);

	const auto neededCommandline = KShell::joinArgs(QStringList{
		descriptor.executable,
	} + KShell::splitArgs(descriptor.arguments) + QStringList{
		"--",
		"%u",
	}).toUtf8();

	const auto currentAppInfo = Gio::AppInfo::get_default_for_type(
		handlerType.toStdString(),
		true);

	if (currentAppInfo) {
		const auto currentCommandline = QString::fromStdString(
			currentAppInfo->get_commandline());

		return currentCommandline == neededCommandline;
	}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

	return false;
}

void RegisterUrlScheme(const UrlSchemeDescriptor &descriptor) {
#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
	try {
		if (KSandbox::isSnap()) {
			SnapDefaultHandler(descriptor.protocol);
			return;
		}

		if (CheckUrlScheme(descriptor)) {
			return;
		}
		UnregisterUrlScheme(descriptor);

		const auto handlerType = QString("x-scheme-handler/%1")
			.arg(descriptor.protocol);

		const auto commandlineForCreator = KShell::joinArgs(QStringList{
			descriptor.executable,
		} + KShell::splitArgs(descriptor.arguments) + QStringList{
			"--",
		}).toUtf8();

		const auto newAppInfo = Gio::AppInfo::create_from_commandline(
			commandlineForCreator.toStdString(),
			descriptor.displayAppName.toStdString(),
			Gio::AppInfo::CreateFlags::SUPPORTS_URIS);

		if (newAppInfo) {
			newAppInfo->set_as_default_for_type(handlerType.toStdString());
		}
	} catch (const std::exception &e) {
		LOG(("Register Url Scheme Error: %1").arg(
			QString::fromStdString(e.what())));
	}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION
}

void UnregisterUrlScheme(const UrlSchemeDescriptor &descriptor) {
#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
	const auto handlerType = QString("x-scheme-handler/%1")
		.arg(descriptor.protocol);

	const auto neededCommandline = KShell::joinArgs(QStringList{
		descriptor.executable,
	} + KShell::splitArgs(descriptor.arguments) + QStringList{
		"--",
		"%u",
	}).toUtf8();

	auto registeredAppInfoList = g_app_info_get_recommended_for_type(
		handlerType.toUtf8().constData());

	for (auto l = registeredAppInfoList; l != nullptr; l = l->next) {
		const auto currentRegisteredAppInfo = reinterpret_cast<GAppInfo*>(
			l->data);

		const auto currentAppInfoId = QString(
			g_app_info_get_id(currentRegisteredAppInfo));

		const auto currentCommandline = QString(
			g_app_info_get_commandline(currentRegisteredAppInfo));

		if (currentCommandline == neededCommandline
			&& currentAppInfoId.startsWith("userapp-")) {
			g_app_info_delete(currentRegisteredAppInfo);
		}
	}

	if (registeredAppInfoList) {
		g_list_free_full(registeredAppInfoList, g_object_unref);
	}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION
}

} // namespace base::Platform
