// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/base_platform_wifi_utilities.h"

#include "base/const_string.h"

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
#include "base/platform/linux/base_linux_glibmm_helper.h"

#include <glibmm.h>
#include <giomm.h>
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

namespace base::Platform {
namespace {

constexpr auto kNMService = "org.freedesktop.NetworkManager"_cs;
constexpr auto kNMObjectPath = "/org/freedesktop/NetworkManager"_cs;
constexpr auto kNMInterface = kNMService;
constexpr auto kPropertiesInterface = "org.freedesktop.DBus.Properties"_cs;

constexpr auto kNoiseFloorDbm = -90.;
constexpr auto kSignalMaxDbm = -20.;

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
std::vector<WiFiNetwork> NMWiFiNetworks() {
	std::vector<WiFiNetwork> result;

	try {
		const auto connection = Gio::DBus::Connection::get_sync(
			Gio::DBus::BusType::BUS_TYPE_SYSTEM);

		auto reply = connection->call_sync(
			std::string(kNMObjectPath),
			std::string(kPropertiesInterface),
			"Get",
			MakeGlibVariant(std::tuple{
				Glib::ustring(std::string(kNMInterface)),
				Glib::ustring("Devices"),
			}),
			std::string(kNMService));

		auto devices = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(
			GlibVariantCast<Glib::VariantBase>(reply.get_child(0)));

		for (auto i = 0; i != devices.get_n_children(); ++i) {
			try {
				auto reply = connection->call_sync(
					GlibVariantCast<Glib::ustring>(devices.get_child(i)),
					std::string(kPropertiesInterface),
					"Get",
					MakeGlibVariant(std::tuple{
						Glib::ustring("org.freedesktop.NetworkManager.Device.Wireless"),
						Glib::ustring("AccessPoints"),
					}),
					std::string(kNMService));

				auto accessPoints = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(
					GlibVariantCast<Glib::VariantBase>(reply.get_child(0)));

				for (auto i = 0; i != accessPoints.get_n_children(); ++i) {
					try {
						const auto getAPProp = [&](const Glib::ustring &prop) {
							auto reply = connection->call_sync(
								GlibVariantCast<Glib::ustring>(accessPoints.get_child(i)),
								std::string(kPropertiesInterface),
								"Get",
								MakeGlibVariant(std::tuple{
									Glib::ustring("org.freedesktop.NetworkManager.AccessPoint"),
									prop,
								}),
								std::string(kNMService));

							return GlibVariantCast<Glib::VariantBase>(
								reply.get_child(0));
						};

						result.push_back({
							QString::fromStdString(
								GlibVariantCast<Glib::ustring>(getAPProp("HwAddress"))).toLower(),
							[&]() -> std::optional<QByteArray> {
								try {
									return QByteArray::fromStdString(
										GlibVariantCast<std::string>(getAPProp("Ssid")));
								} catch (...) {
									return std::nullopt;
								}
							}(),
							[&] {
								const auto result = GlibVariantCast<int>(getAPProp("LastSeen"));
								return (result >= 0) ? std::make_optional(result) : std::nullopt;
							}(),
							std::nullopt,
							GlibVariantCast<uint>(getAPProp("Frequency")),
							-((-float64(GlibVariantCast<guint8>(getAPProp("Strength"))) + 100.)
								/ 70.
								* (kSignalMaxDbm - kNoiseFloorDbm))
								+ kSignalMaxDbm,
							std::nullopt,
						});
					} catch (...) {
					}
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

std::vector<WiFiNetwork> WiFiNetworks() {
#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
	const auto nmResult = NMWiFiNetworks();
	if (!nmResult.empty()) {
		return nmResult;
	}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

	return {};
}

} // namespace base::Platform
