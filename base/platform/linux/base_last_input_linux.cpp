// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_last_input_linux.h"

#include "base/integration.h"
#include "base/platform/base_platform_info.h"

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
#include "base/platform/linux/base_linux_xcb_utilities.h"
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusError>
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
#include <xcb/screensaver.h>
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

namespace base::Platform {
namespace {

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
std::optional<crl::time> XCBLastUserInputTime() {
	const auto connection = XCB::GetConnectionFromQt();
	if (!connection) {
		return std::nullopt;
	}

	if (!XCB::IsExtensionPresent(connection, &xcb_screensaver_id)) {
		return std::nullopt;
	}

	const auto root = XCB::GetRootWindowFromQt();
	if (!root.has_value()) {
		return std::nullopt;
	}

	const auto cookie = xcb_screensaver_query_info(
		connection,
		*root);

	auto reply = xcb_screensaver_query_info_reply(
		connection,
		cookie,
		nullptr);

	if (!reply) {
		return std::nullopt;
	}

	const auto idle = reply->ms_since_user_input;
	free(reply);

	return (crl::now() - static_cast<crl::time>(idle));
}
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
std::optional<crl::time> FreedesktopDBusLastUserInputTime() {
	static auto NotSupported = false;

	if (NotSupported) {
		return std::nullopt;
	}

	static const auto Message = QDBusMessage::createMethodCall(
		"org.freedesktop.ScreenSaver",
		"/org/freedesktop/ScreenSaver",
		"org.freedesktop.ScreenSaver",
		"GetSessionIdleTime");

	const QDBusReply<uint> reply = QDBusConnection::sessionBus().call(
		Message);

	static const auto NotSupportedErrors = {
		QDBusError::Disconnected,
		QDBusError::ServiceUnknown,
		QDBusError::NotSupported,
	};

	static const auto NotSupportedErrorsToLog = {
		QDBusError::AccessDenied,
	};

	if (reply.isValid()) {
		return (crl::now() - static_cast<crl::time>(reply.value()));
	} else if (ranges::contains(NotSupportedErrors, reply.error().type())) {
		NotSupported = true;
	} else {
		if (ranges::contains(NotSupportedErrorsToLog, reply.error().type())) {
			NotSupported = true;
		}

		Integration::Instance().logMessage(
			QString("Unable to get last user input time "
				"from org.freedesktop.ScreenSaver: %1: %2")
				.arg(reply.error().name())
				.arg(reply.error().message()));
	}

	return std::nullopt;
}

std::optional<crl::time> MutterDBusLastUserInputTime() {
	static auto NotSupported = false;

	if (NotSupported) {
		return std::nullopt;
	}

	static const auto Message = QDBusMessage::createMethodCall(
		"org.gnome.Mutter.IdleMonitor",
		"/org/gnome/Mutter/IdleMonitor/Core",
		"org.gnome.Mutter.IdleMonitor",
		"GetIdletime");

	const QDBusReply<qulonglong> reply = QDBusConnection::sessionBus().call(
		Message);

	static const auto NotSupportedErrors = {
		QDBusError::Disconnected,
		QDBusError::ServiceUnknown,
	};

	static const auto NotSupportedErrorsToLog = {
		QDBusError::AccessDenied,
	};

	if (reply.isValid()) {
		return (crl::now() - static_cast<crl::time>(reply.value()));
	} else if (ranges::contains(NotSupportedErrors, reply.error().type())) {
		NotSupported = true;
	} else {
		if (ranges::contains(NotSupportedErrorsToLog, reply.error().type())) {
			NotSupported = true;
		}

		Integration::Instance().logMessage(
			QString("Unable to get last user input time "
				"from org.gnome.Mutter.IdleMonitor: %1: %2")
				.arg(reply.error().name())
				.arg(reply.error().message()));
	}

	return std::nullopt;
}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

} // namespace

std::optional<crl::time> LastUserInputTime() {
#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
	if (!::Platform::IsWayland()) {
		const auto xcbResult = XCBLastUserInputTime();
		if (xcbResult.has_value()) {
			return xcbResult;
		}
	}
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
	const auto freedesktopResult = FreedesktopDBusLastUserInputTime();
	if (freedesktopResult.has_value()) {
		return freedesktopResult;
	}

	const auto mutterResult = MutterDBusLastUserInputTime();
	if (mutterResult.has_value()) {
		return mutterResult;
	}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

	return std::nullopt;
}

} // namespace base::Platform
