/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include <giomm/applaunchcontext.h>

namespace base::Platform {

Glib::RefPtr<Gio::AppLaunchContext> AppLaunchContext();

} // namespace base::Platform
