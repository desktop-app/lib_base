// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/base_platform_bt_utilities.h"

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
#include "base/platform/linux/base_linux_glibmm_helper.h"

#include <glibmm.h>
#include <giomm.h>
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

namespace base::Platform {
namespace {

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
std::vector<BTDevice> BluezBTDevices() {
	std::vector<BTDevice> result;

	try {
		const auto connection = Gio::DBus::Connection::get_sync(
			Gio::DBus::BusType::BUS_TYPE_SYSTEM);

		auto reply = connection->call_sync(
			"/",
			"org.freedesktop.DBus.ObjectManager",
			"GetManagedObjects",
			{},
			"org.bluez");

		auto objects = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(
			reply.get_child(0));

		for (auto i = 0; i != objects.get_n_children(); ++i) {
			try {
				auto object = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(
					objects.get_child(i));

				const auto interfaces = GlibVariantCast<
					std::map<
						Glib::ustring,
						std::map<
							Glib::ustring,
							Glib::VariantBase>>>(object.get_child(1));
				
				for (const auto &[interface, properties] : interfaces) {
					if (interface != "org.bluez.Device1") {
						continue;
					}

					result.push_back({
						QString::fromStdString(
							GlibVariantCast<Glib::ustring>(properties.at("Address"))).toLower(),
						[&]() -> std::optional<QString> {
							try {
								return QString::fromStdString(
										GlibVariantCast<Glib::ustring>(properties.at("Name")));
							} catch (...) {
								return std::nullopt;
							}
						}(),
						std::nullopt,
						GlibVariantCast<gint16>(properties.at("RSSI")),
					});
				}
			} catch (...) {
			}
		}
	} catch (...) {
	}

	return result;
}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

} // namespace

std::vector<BTDevice> BTDevices() {
#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
	const auto bluezResult = BluezBTDevices();
	if (!bluezResult.empty()) {
		return bluezResult;
	}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

	return {};
}

} // namespace base::Platform
