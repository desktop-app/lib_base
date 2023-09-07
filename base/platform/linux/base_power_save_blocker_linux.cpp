// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_power_save_blocker_linux.h"

#include "base/platform/base_platform_info.h"
#include "base/platform/linux/base_linux_xdp_utilities.h"
#include "base/platform/linux/base_linux_wayland_integration.h"
#include "base/timer_rpl.h"
#include "base/random.h"

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
#include "base/platform/linux/base_linux_xcb_utilities.h"
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

#include <QtGui/QWindow>

#include <giomm.h>

namespace base::Platform {
namespace {

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
constexpr auto kResetScreenSaverTimeout = 10 * crl::time(1000);

// Use the basic reset API
// due to https://gitlab.freedesktop.org/xorg/xserver/-/issues/363
void XCBPreventDisplaySleep(bool prevent) {
	static rpl::lifetime lifetime;
	if (!prevent) {
		lifetime.destroy();
		return;
	} else if (lifetime) {
		return;
	}

	base::timer_each(
		kResetScreenSaverTimeout
	) | rpl::start_with_next([connection = XCB::Connection()] {
		if (!connection || xcb_connection_has_error(connection)) {
			return;
		}
		xcb_force_screen_saver(connection, XCB_SCREEN_SAVER_RESET);
	}, lifetime);
}
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

/* https://flatpak.github.io/xdg-desktop-portal/#gdbus-org.freedesktop.portal.Inhibit
 * 1: Logout
 * 2: User Switch
 * 4: Suspend
 * 8: Idle
 */
void PortalInhibit(
	Glib::ustring &requestPath,
	uint flags = 0,
	const QString &description = {},
	QPointer<QWindow> window = {}) {
	try {
		const auto connection = Gio::DBus::Connection::get_sync(
			Gio::DBus::BusType::SESSION);

		if (!requestPath.empty()) {
			connection->call(
				requestPath,
				XDP::kRequestInterface,
				"Close",
				{},
				{},
				XDP::kService);
			requestPath = "";
			return;
		}

		const auto handleToken = Glib::ustring("desktop_app")
			+ std::to_string(base::RandomValue<uint>());

		auto uniqueName = connection->get_unique_name();
		uniqueName.erase(0, 1);
		uniqueName.replace(uniqueName.find('.'), 1, 1, '_');

		requestPath = Glib::ustring(
				"/org/freedesktop/portal/desktop/request/")
			+ uniqueName
			+ '/'
			+ handleToken;

		connection->call(
			XDP::kObjectPath,
			"org.freedesktop.portal.Inhibit",
			"Inhibit",
			Glib::create_variant(std::tuple{
				XDP::ParentWindowID(window),
				flags,
				std::map<Glib::ustring, Glib::VariantBase>{
					{
						"handle_token",
						Glib::create_variant(handleToken)
					},
					{
						"reason",
						Glib::create_variant(
							Glib::ustring(description.toStdString()))
					},
				},
			}),
			{},
			XDP::kService);
	} catch (...) {
	}
}

void PortalPreventDisplaySleep(
	bool prevent,
	const QString &description = {},
	QPointer<QWindow> window = {}) {
	static Glib::ustring requestPath;
	if (prevent && !requestPath.empty()) {
		return;
	}
	PortalInhibit(requestPath, 8 /* Idle */, description, window);
}

void PortalPreventAppSuspension(
	bool prevent,
	const QString &description = {},
	QPointer<QWindow> window = {}) {
	static Glib::ustring requestPath;
	if (prevent && !requestPath.empty()) {
		return;
	}
	PortalInhibit(requestPath, 4 /* Suspend */, description, window);
}

} // namespace

void BlockPowerSave(
	PowerSaveBlockType type,
	const QString &description,
	QPointer<QWindow> window) {
	switch (type) {
	case PowerSaveBlockType::PreventAppSuspension:
		PortalPreventAppSuspension(true, description, window);
		break;
	case PowerSaveBlockType::PreventDisplaySleep:
		if (const auto integration = WaylandIntegration::Instance()) {
			integration->preventDisplaySleep(true, window);
		}
#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
		XCBPreventDisplaySleep(true);
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION
		PortalPreventDisplaySleep(true, description, window);
		break;
	}
}

void UnblockPowerSave(PowerSaveBlockType type, QPointer<QWindow> window) {
	switch (type) {
	case PowerSaveBlockType::PreventAppSuspension:
		PortalPreventAppSuspension(false);
		break;
	case PowerSaveBlockType::PreventDisplaySleep:
		if (const auto integration = WaylandIntegration::Instance()) {
			integration->preventDisplaySleep(false, window);
		}
#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
		XCBPreventDisplaySleep(false);
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION
		PortalPreventDisplaySleep(false);
		break;
	}
}

} // namespace base::Platform
