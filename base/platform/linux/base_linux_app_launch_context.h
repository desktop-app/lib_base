/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

namespace gi::repository::Gio {
class AppLaunchContext;
} // namespace gi::repository::Gio

namespace base::Platform {

gi::repository::Gio::AppLaunchContext AppLaunchContext();

} // namespace base::Platform
