/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

namespace base {
namespace Platform {

class XErrorHandlerRestorer {
public:
	XErrorHandlerRestorer();
	~XErrorHandlerRestorer();

private:
	class Private;
	const std::unique_ptr<Private> _private;
};

} // namespace Platform
} // namespace base
