/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "base/file_lock.h"

namespace base {

class CrashReportWriter final {
public:
	CrashReportWriter(const QString &path);
	~CrashReportWriter();

	void start();

	void addAnnotation(std::string key, std::string value);

private:
	[[nodiscard]] QString reportPath() const;
	[[nodiscard]] std::optional<QByteArray> readPreviousReport();
	bool openReport();
	void closeReport();
	void startCatching();
	void finishCatching();

	const QString _path;
	FileLock _reportLock;
	QFile _reportFile;
	std::optional<QByteArray> _previousReport;
	std::map<std::string, std::string> _annotations;

};

} // namespace base
