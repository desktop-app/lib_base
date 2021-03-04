// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_url_scheme_linux.h"

#include "base/integration.h"

#include <QtCore/QFile>

#include <gio/gio.h>
#include <glibmm.h>
#include <giomm.h>

namespace base::Platform {
namespace {

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
		Integration::Instance().logMessage(QString::fromStdString(e.what()));
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
