// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_url_scheme_linux.h"

#include "base/platform/linux/base_linux_glibmm_helper.h"
#include "base/const_string.h"
#include "base/debug_log.h"

#include <QtCore/QFile>
#include <QtCore/QEventLoop>
#include <QtGui/QWindow>

#include <private/qguiapplication_p.h>
#include <gio/gio.h>
#include <glibmm.h>
#include <giomm.h>

namespace base::Platform {
namespace {

constexpr auto kSnapcraftSettingsService = "io.snapcraft.Settings"_cs;
constexpr auto kSnapcraftSettingsObjectPath = "/io/snapcraft/Settings"_cs;
constexpr auto kSnapcraftSettingsInterface = kSnapcraftSettingsService;

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

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
class SnapDefaultHandler : public QWindow {
public:
	SnapDefaultHandler(const QString &protocol);
};

SnapDefaultHandler::SnapDefaultHandler(const QString &protocol) {
	try {
		const auto connection = Gio::DBus::Connection::get_sync(
			Gio::DBus::BusType::BUS_TYPE_SESSION);

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

		if (currentHandler == expectedHandler.toStdString()) {
			return;
		}

		QEventLoop loop;

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
				} catch (const Glib::Error &e) {
					LOG(("Snap Default Handler Error: %1")
						.arg(QString::fromStdString(e.what())));
				}

				loop.quit();
			},
			std::string(kSnapcraftSettingsService));

		QGuiApplicationPrivate::showModalWindow(this);
		loop.exec();
		QGuiApplicationPrivate::hideModalWindow(this);
	} catch (const Glib::Error &e) {
		LOG(("Snap Default Handler Error: %1")
			.arg(QString::fromStdString(e.what())));
	}
}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

} // namespace

bool CheckUrlScheme(const UrlSchemeDescriptor &descriptor) {
	try {
		const auto handlerType = QString("x-scheme-handler/%1")
			.arg(descriptor.protocol);

		const auto neededCommandline = QString("%1 -- %u")
			.arg(QString(
				EscapeShell(QFile::encodeName(descriptor.executable))));

		const auto currentAppInfo = Gio::AppInfo::get_default_for_type(
			handlerType.toStdString(),
			true);

		if (currentAppInfo) {
			const auto currentCommandline = QString::fromStdString(
				currentAppInfo->get_commandline());

			return currentCommandline == neededCommandline;
		}
	} catch (...) {
	}

	return false;
}

void RegisterUrlScheme(const UrlSchemeDescriptor &descriptor) {
	try {
#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
		if (qEnvironmentVariableIsSet("SNAP")) {
			SnapDefaultHandler(descriptor.protocol);
			return;
		}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

		if (CheckUrlScheme(descriptor)) {
			return;
		}
		UnregisterUrlScheme(descriptor);

		const auto handlerType = QString("x-scheme-handler/%1")
			.arg(descriptor.protocol);

		const auto commandlineForCreator = QString("%1 --")
			.arg(QString(
				EscapeShell(QFile::encodeName(descriptor.executable))));

		const auto newAppInfo = Gio::AppInfo::create_from_commandline(
			commandlineForCreator.toStdString(),
			descriptor.displayAppName.toStdString(),
			Gio::AppInfoCreateFlags::APP_INFO_CREATE_SUPPORTS_URIS);

		if (newAppInfo) {
			newAppInfo->set_as_default_for_type(handlerType.toStdString());
		}
	} catch (const Glib::Error &e) {
		LOG(("Register Url Scheme Error: %1").arg(QString::fromStdString(e.what())));
	}
}

void UnregisterUrlScheme(const UrlSchemeDescriptor &descriptor) {
	const auto handlerType = QString("x-scheme-handler/%1")
		.arg(descriptor.protocol);

	const auto neededCommandline = QString("%1 -- %u")
		.arg(QString(EscapeShell(QFile::encodeName(descriptor.executable))));

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
}

} // namespace base::Platform
