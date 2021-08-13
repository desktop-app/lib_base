/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "base/basic_types.h"

#include <QtCore/QLibrary>
#include <optional>
#include <memory>

namespace base {
namespace Platform {

class GtkIntegration {
public:
	static GtkIntegration *Instance();

	QLibrary &library() {
		return _lib;
	}

	void load(const QString &allowedBackends, bool force = false);
	int exec(const QString &parentDBusName);
	[[nodiscard]] bool loaded() const;

	[[nodiscard]] static QString ServiceName();
	static void SetServiceName(const QString &serviceName);

	[[nodiscard]] bool checkVersion(
		uint major,
		uint minor,
		uint micro) const;

	[[nodiscard]] std::optional<bool> getBoolSetting(
		const QString &propertyName) const;

	[[nodiscard]] std::optional<int> getIntSetting(
		const QString &propertyName) const;

	[[nodiscard]] std::optional<QString> getStringSetting(
		const QString &propertyName) const;

	void connectToSetting(
		const QString &propertyName,
		Fn<void()> callback);

private:
	GtkIntegration();
	~GtkIntegration();

	class Private;
	const std::unique_ptr<Private> _private;

	QLibrary _lib;
};

} // namespace Platform
} // namespace base
