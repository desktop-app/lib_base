// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_url_scheme_linux.h"

#include "base/integration.h"

#include <QtCore/QFile>

extern "C" {
#undef signals
#include <gio/gio.h>
#define signals public
} // extern "C"

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
	const auto handlerType = QString("x-scheme-handler/%1")
		.arg(descriptor.protocol);

	const auto neededCommandline = QString("%1 -- %u")
		.arg(QString(EscapeShell(QFile::encodeName(descriptor.executable))));

	auto currentAppInfo = g_app_info_get_default_for_type(
		handlerType.toUtf8(),
		true);

	if (currentAppInfo) {
		const auto currentCommandline = QString(
			g_app_info_get_commandline(currentAppInfo));

		g_object_unref(currentAppInfo);

		return currentCommandline == neededCommandline;
	}

	return false;
}

void RegisterUrlScheme(const UrlSchemeDescriptor &descriptor) {
	if (CheckUrlScheme(descriptor)) {
		return;
	}
	UnregisterUrlScheme(descriptor);

	GError *error = nullptr;

	const auto handlerType = QString("x-scheme-handler/%1")
		.arg(descriptor.protocol);

	const auto commandlineForCreator = QString("%1 --")
		.arg(QString(EscapeShell(QFile::encodeName(descriptor.executable))));

	auto newAppInfo = g_app_info_create_from_commandline(
		commandlineForCreator.toUtf8(),
		descriptor.displayAppName.toUtf8(),
		G_APP_INFO_CREATE_SUPPORTS_URIS,
		&error);

	if (newAppInfo) {
		g_app_info_set_as_default_for_type(
			newAppInfo,
			handlerType.toUtf8(),
			&error);

		g_object_unref(newAppInfo);
	}

	if (error) {
		Integration::Instance().logMessage(error->message);

		g_error_free(error);
	}
}

void UnregisterUrlScheme(const UrlSchemeDescriptor &descriptor) {
	const auto handlerType = QString("x-scheme-handler/%1")
		.arg(descriptor.protocol);

	const auto neededCommandline = QString("%1 -- %u")
		.arg(QString(EscapeShell(QFile::encodeName(descriptor.executable))));

	auto registeredAppInfoList = g_app_info_get_recommended_for_type(
		handlerType.toUtf8());

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
