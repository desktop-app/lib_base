/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "base/platform/linux/base_linux_app_launch_context.h"

#include "base/platform/linux/base_linux_xdp_utilities.h"
#include "base/platform/linux/base_linux_xdg_activation_token.h"

#include <gio/gio.hpp>

namespace base::Platform {
namespace internal {
namespace {

using namespace gi::repository;

class AppLaunchContext : public Gio::impl::AppLaunchContextImpl {
public:
	AppLaunchContext() : Gio::impl::AppLaunchContextImpl(this) {
		if (const auto parentWindowId = XDP::ParentWindowID()
				; !parentWindowId.empty()) {
			setenv("PARENT_WINDOW_ID", parentWindowId);
		}
	}

	gi::cstring get_startup_notify_id_(
			Gio::AppInfo,
			gi::Collection<GList, ::GFile*, gi::transfer_none_t>
	) noexcept override {
		if (const auto token = XdgActivationToken(); !token.isNull()) {
			return token.toStdString();
		}
		return {};
	}
};

} // namespace
} // namespace internal

gi::repository::Gio::AppLaunchContext AppLaunchContext() {
	return *gi::make_ref<internal::AppLaunchContext>();
}

} // namespace base::Platform
