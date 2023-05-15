/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

namespace Gio {
class AppLaunchContext;
} // namespace Gio

namespace base::Platform {

std::shared_ptr<Gio::AppLaunchContext> AppLaunchContext();

} // namespace base::Platform
