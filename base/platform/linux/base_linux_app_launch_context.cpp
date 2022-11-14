/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "base/platform/linux/base_linux_app_launch_context.h"

#include "base/platform/linux/base_linux_wayland_integration.h"

#include <giomm.h>

typedef GAppLaunchContext DesktopAppAppLaunchContext;
typedef GAppLaunchContextClass DesktopAppAppLaunchContextClass;

G_DEFINE_TYPE(
	DesktopAppAppLaunchContext,
	desktop_app_app_launch_context,
	G_TYPE_APP_LAUNCH_CONTEXT)

static void desktop_app_app_launch_context_class_init(
		DesktopAppAppLaunchContextClass *klass) {
	const auto ctx_class = G_APP_LAUNCH_CONTEXT_CLASS(klass);
	ctx_class->get_startup_notify_id = [](
			GAppLaunchContext *context,
			GAppInfo*,
			GList*) -> char* {
		if (const auto token = g_environ_getenv(
			g_app_launch_context_get_environment(context),
			"XDG_ACTIVATION_TOKEN")) {
			return strdup(token);
		}
		return nullptr;
	};
}

static void desktop_app_app_launch_context_init(
		DesktopAppAppLaunchContext *ctx) {
	using base::Platform::WaylandIntegration;
	if (const auto integration = WaylandIntegration::Instance()) {
		if (const auto token = integration->activationToken()
			; !token.isNull()) {
			g_app_launch_context_setenv(
				G_APP_LAUNCH_CONTEXT(ctx),
				"XDG_ACTIVATION_TOKEN",
				token.toUtf8().constData());
		}
	}
}

namespace base::Platform {

Glib::RefPtr<Gio::AppLaunchContext> AppLaunchContext() {
	return Glib::wrap(
		G_APP_LAUNCH_CONTEXT(
			g_object_new(
				desktop_app_app_launch_context_get_type(),
				nullptr)));
}

} // namespace base::Platform
