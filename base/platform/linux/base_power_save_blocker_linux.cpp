// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_power_save_blocker_linux.h"

#include "base/platform/base_platform_info.h"
#include "base/platform/linux/base_linux_xdp_utilities.h"
#include "base/timer_rpl.h"
#include "base/weak_ptr.h"
#include "base/random.h"

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
#include "base/platform/linux/base_linux_xcb_utilities.h"
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

#include <QtGui/QWindow>

#include <xdpinhibit/xdpinhibit.hpp>
#include <xdprequest/xdprequest.hpp>

namespace base::Platform {
namespace {

using namespace gi::repository;
namespace GObject = gi::repository::GObject;

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
	) | rpl::map([connection = XCB::Connection()] {
		return connection;
	}) | rpl::filter([](xcb_connection_t *connection) {
		return connection && !xcb_connection_has_error(connection);
	}) | rpl::on_next([](xcb_connection_t *connection) {
		free(
			xcb_request_check(
				connection,
				xcb_force_screen_saver_checked(
					connection,
					XCB_SCREEN_SAVER_RESET)));
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
	PortalInhibit(uint flags = 0, const QString &description = {}) {
		XdpInhibit::InhibitProxy::new_for_bus(
			Gio::BusType::SESSION_,
			Gio::DBusProxyFlags::NONE_,
			XDP::kService,
			XDP::kObjectPath,
			crl::guard(this, [=](GObject::Object, Gio::AsyncResult res) {
				auto proxy = XdpInhibit::InhibitProxy::new_for_bus_finish(
					res,
					nullptr);

				if (!proxy) {
					return;
				}

				const auto handleToken = "desktop_app"
					+ std::to_string(RandomValue<uint>());

				auto uniqueName = std::string(
					proxy.get_connection().get_unique_name());

				uniqueName.erase(0, 1);
				uniqueName.replace(uniqueName.find('.'), 1, 1, '_');

				XdpRequest::RequestProxy::new_(
					proxy.get_connection(),
					Gio::DBusProxyFlags::NONE_,
					XDP::kService,
					XDP::kObjectPath
						+ std::string("/request/")
						+ uniqueName
						+ '/'
						+ handleToken,
					nullptr,
					crl::guard(this, [=](
							GObject::Object,
							Gio::AsyncResult res) mutable {
						_request = XdpRequest::RequestProxy::new_finish(
							res,
							nullptr);

						auto request = _request; // take a ref
						const auto weak = make_weak(this);
						const auto signalId = std::make_shared<ulong>();
						*signalId = _request.signal_response().connect([=](
								XdpRequest::Request,
								guint,
								GLib::Variant) mutable {
							if (!weak) {
								request.call_close(nullptr);
							}
							request.disconnect(*signalId);
						});

						XdpInhibit::Inhibit(proxy).call_inhibit(
							XDP::ParentWindowID(),
							flags,
							GLib::Variant::new_array({
								GLib::Variant::new_dict_entry(
									GLib::Variant::new_string("handle_token"),
									GLib::Variant::new_variant(
										GLib::Variant::new_string(
											handleToken))),
								GLib::Variant::new_dict_entry(
									GLib::Variant::new_string("reason"),
									GLib::Variant::new_variant(
										GLib::Variant::new_string(
											description.toStdString()))),
							}),
							nullptr);
					}));
			}));
	}

	~PortalInhibit() {
		if (!_request) {
			return;
		}

		_request.call_close(nullptr);
	}

private:
	XdpRequest::Request _request;
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
#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
			XCBPreventDisplaySleep(false);
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION
			PortalPreventDisplaySleep(false);
			break;
		}
	});
}

} // namespace base::Platform
