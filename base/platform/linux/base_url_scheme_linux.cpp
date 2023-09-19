// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_url_scheme_linux.h"

#include "base/integration.h"
#include "base/debug_log.h"

#include <QtGui/QGuiApplication>
#include <QtWidgets/QWidget>

#include <kshell.h>
#include <ksandbox.h>

#include <glibmm.h>
#include <giomm.h>

namespace base::Platform {
namespace {

constexpr auto kSnapcraftSettingsService = "io.snapcraft.Settings";
constexpr auto kSnapcraftSettingsObjectPath = "/io/snapcraft/Settings";
constexpr auto kSnapcraftSettingsInterface = kSnapcraftSettingsService;

void SnapDefaultHandler(const QString &protocol) {
	const auto connection = [] {
		try {
			return Gio::DBus::Connection::get_sync(
				Gio::DBus::BusType::SESSION);
		} catch (const std::exception &e) {
			LOG(("Snap Default Handler Error: %1").arg(e.what()));
			return Glib::RefPtr<Gio::DBus::Connection>();
		}
	}();

	if (!connection) {
		return;
	}

	connection->call(
		kSnapcraftSettingsObjectPath,
		kSnapcraftSettingsInterface,
		"GetSub",
		Glib::create_variant(std::tuple{
			Glib::ustring("default-url-scheme-handler"),
			Glib::ustring(protocol.toStdString()),
		}),
		[=](const Glib::RefPtr<Gio::AsyncResult> &result) {
			const auto currentHandler = [&]() -> std::optional<Glib::ustring> {
				try {
					return connection->call_finish(
						result
					).get_child(0).get_dynamic<Glib::ustring>();
				} catch (const std::exception &e) {
					LOG(("Snap Default Handler Error: %1").arg(e.what()));
					return std::nullopt;
				}
			}();

			if (!currentHandler) {
				return;
			}

			const auto &integration = Integration::Instance();
			const auto expectedHandler = integration.executableName()
				+ u".desktop"_q;

			if (currentHandler->c_str() == expectedHandler) {
				return;
			}

			const auto window = std::make_shared<QWidget>();
			window->setAttribute(Qt::WA_DontShowOnScreen);
			window->setWindowModality(Qt::ApplicationModal);
			window->show();

			connection->call(
				kSnapcraftSettingsObjectPath,
				kSnapcraftSettingsInterface,
				"SetSub",
				Glib::create_variant(std::tuple{
					Glib::ustring("default-url-scheme-handler"),
					Glib::ustring(protocol.toStdString()),
					Glib::ustring(expectedHandler.toStdString()),
				}),
				[=](const Glib::RefPtr<Gio::AsyncResult> &result) {
					(void)window; // don't destroy until finish
					try {
						connection->call_finish(result);
					} catch (const std::exception &e) {
						LOG(("Snap Default Handler Error: %1").arg(e.what()));
					}
				},
				kSnapcraftSettingsService);
		},
		kSnapcraftSettingsService);
}

} // namespace

bool CheckUrlScheme(const UrlSchemeDescriptor &descriptor) {
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

	return false;
}

void RegisterUrlScheme(const UrlSchemeDescriptor &descriptor) {
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

	const auto appId = QGuiApplication::desktopFileName().toStdString();
	if (!appId.empty()) {
		if (const auto appInfo = Gio::DesktopAppInfo::create(
			appId + ".desktop")) {
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
		LOG(("Register Url Scheme Error: %1").arg(e.what()));
	}
}

void UnregisterUrlScheme(const UrlSchemeDescriptor &descriptor) {
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
}

} // namespace base::Platform
