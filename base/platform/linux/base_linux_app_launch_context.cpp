/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "base/platform/linux/base_linux_app_launch_context.h"

#include "base/platform/linux/base_linux_wayland_integration.h"
#include "base/platform/linux/base_linux_xdp_utilities.h"

#include <QtGui/QGuiApplication>

#include <glibmm.h>
#include <gio/gio.hpp>

namespace base::Platform {
namespace internal {
namespace {

using namespace gi::repository;

class AppLaunchContext : public Gio::impl::AppLaunchContextImpl {
public:
	AppLaunchContext() : Gio::impl::AppLaunchContextImpl(this) {
		if (const auto integration = WaylandIntegration::Instance()) {
			if (const auto token = integration->activationToken()
				; !token.isNull()) {
				setenv("XDG_ACTIVATION_TOKEN", token.toStdString());
			}
		}
		if (const auto parentWindowId = XDP::ParentWindowID()
			; !parentWindowId.empty()) {
			setenv("PARENT_WINDOW_ID", parentWindowId);
		}
	}

	char *get_startup_notify_id_(GAppInfo *info, GList*) noexcept override {
		if (const auto desktopId = g_app_info_get_id(info)) {
			if (const auto integration = WaylandIntegration::Instance()) {
				if (const auto token = integration->activationToken(
					QString::fromUtf8(desktopId).chopped(8)).toUtf8()
					; !token.isNull()) {
					return strdup(token.constData());
				}
			}
		}
		if (const auto token = GLib::environ_getenv(
			get_environment(),
			"XDG_ACTIVATION_TOKEN")) {
			return strdup(token->c_str());
		}
		return nullptr;
	}
};

} // namespace
} // namespace internal

gi::repository::Gio::AppLaunchContext AppLaunchContext() {
	return *gi::make_ref<internal::AppLaunchContext>();
}

} // namespace base::Platform
