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
#include "base/weak_ptr.h"
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

	timer_each(
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
class PortalInhibit final : public has_weak_ptr {
public:
	PortalInhibit(uint flags = 0, const QString &description = {})
	: _dbusConnection([] {
		try {
			return Gio::DBus::Connection::get_sync(
				Gio::DBus::BusType::SESSION);
		} catch (...) {
			return Glib::RefPtr<Gio::DBus::Connection>();
		}
	}()) {
		if (!_dbusConnection) {
			return;
		}

		const auto handleToken = Glib::ustring("desktop_app")
			+ std::to_string(RandomValue<uint>());

		auto uniqueName = _dbusConnection->get_unique_name();
		uniqueName.erase(0, 1);
		uniqueName.replace(uniqueName.find('.'), 1, 1, '_');

		_requestPath = Glib::ustring(
				"/org/freedesktop/portal/desktop/request/")
			+ uniqueName
			+ '/'
			+ handleToken;

		const auto weak = make_weak(this);
		const auto connection = _dbusConnection; // take a ref
		const auto requestPath = _requestPath;
		const auto signalId = std::make_shared<uint>();
		*signalId = _dbusConnection->signal_subscribe(
			[=](
					const Glib::RefPtr<Gio::DBus::Connection> &,
					const Glib::ustring &,
					const Glib::ustring &,
					const Glib::ustring &,
					const Glib::ustring &,
					const Glib::VariantContainerBase &) {
				if (!weak) {
					Close(connection, requestPath);
				}
				connection->signal_unsubscribe(*signalId);
			},
			XDP::kService,
			XDP::kRequestInterface,
			"Response",
			_requestPath);

		_dbusConnection->call(
			XDP::kObjectPath,
			"org.freedesktop.portal.Inhibit",
			"Inhibit",
			Glib::create_variant(std::tuple{
				XDP::ParentWindowID(),
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
	}

	~PortalInhibit() {
		if (!_dbusConnection) {
			return;
		}

		Close(_dbusConnection, _requestPath);
	}

private:
	static void Close(
			const Glib::RefPtr<Gio::DBus::Connection> &connection,
			const Glib::ustring &requestPath) {
		connection->call(
			requestPath,
			XDP::kRequestInterface,
			"Close",
			{},
			{},
			XDP::kService);
	}

	Glib::RefPtr<Gio::DBus::Connection> _dbusConnection;
	Glib::ustring _requestPath;
};

void PortalPreventDisplaySleep(
		bool prevent,
		const QString &description = {}) {
	static std::optional<PortalInhibit> instance;
	if (prevent && !instance) {
		instance.emplace(8 /* Idle */, description);
	} else if (!prevent && instance) {
		instance.reset();
	}
}

void PortalPreventAppSuspension(
		bool prevent,
		const QString &description = {}) {
	static std::optional<PortalInhibit> instance;
	if (prevent && !instance) {
		instance.emplace(4 /* Suspend */, description);
	} else if (!prevent && instance) {
		instance.reset();
	}
}

} // namespace

void BlockPowerSave(
		PowerSaveBlockType type,
		const QString &description,
		QPointer<QWindow> window) {
	crl::on_main([=] {
		switch (type) {
		case PowerSaveBlockType::PreventAppSuspension:
			PortalPreventAppSuspension(true, description);
			break;
		case PowerSaveBlockType::PreventDisplaySleep:
			if (window) {
				if (const auto integration = WaylandIntegration::Instance()) {
					integration->preventDisplaySleep(window, true);
				}
			}
#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
			XCBPreventDisplaySleep(true);
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION
			PortalPreventDisplaySleep(true, description);
			break;
		}
	});
}

void UnblockPowerSave(PowerSaveBlockType type, QPointer<QWindow> window) {
	crl::on_main([=] {
		switch (type) {
		case PowerSaveBlockType::PreventAppSuspension:
			PortalPreventAppSuspension(false);
			break;
		case PowerSaveBlockType::PreventDisplaySleep:
			if (window) {
				if (const auto integration = WaylandIntegration::Instance()) {
					integration->preventDisplaySleep(window, false);
				}
			}
#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
			XCBPreventDisplaySleep(false);
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION
			PortalPreventDisplaySleep(false);
			break;
		}
	});
}

} // namespace base::Platform
