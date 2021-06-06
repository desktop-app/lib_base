// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/base_platform_cellular_utilities.h"

#include "base/const_string.h"

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
#include "base/platform/linux/base_linux_glibmm_helper.h"

#include <glibmm.h>
#include <giomm.h>
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

namespace base::Platform {
namespace {

constexpr auto kMMService = "org.freedesktop.ModemManager1"_cs;

// copied from https://gitlab.freedesktop.org/mobile-broadband/ModemManager/-/blob/master/include/ModemManager-enums.h
typedef enum {
	MM_MODEM_MODE_NONE = 0,
	MM_MODEM_MODE_CS   = 1 << 0,
	MM_MODEM_MODE_2G   = 1 << 1,
	MM_MODEM_MODE_3G   = 1 << 2,
	MM_MODEM_MODE_4G   = 1 << 3,
	MM_MODEM_MODE_5G   = 1 << 4,
	MM_MODEM_MODE_ANY  = 0xFFFFFFFF
} MMModemMode;

typedef enum {
	MM_MODEM_LOCATION_SOURCE_NONE          = 0,
	MM_MODEM_LOCATION_SOURCE_3GPP_LAC_CI   = 1 << 0,
	MM_MODEM_LOCATION_SOURCE_GPS_RAW       = 1 << 1,
	MM_MODEM_LOCATION_SOURCE_GPS_NMEA      = 1 << 2,
	MM_MODEM_LOCATION_SOURCE_CDMA_BS       = 1 << 3,
	MM_MODEM_LOCATION_SOURCE_GPS_UNMANAGED = 1 << 4,
	MM_MODEM_LOCATION_SOURCE_AGPS_MSA      = 1 << 5,
	MM_MODEM_LOCATION_SOURCE_AGPS_MSB      = 1 << 6,
} MMModemLocationSource;

#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
std::vector<CellNetwork> MMCellNetworks() {
	std::vector<CellNetwork> result;

	try {
		const auto connection = Gio::DBus::Connection::get_sync(
			Gio::DBus::BusType::BUS_TYPE_SYSTEM);

		auto reply = connection->call_sync(
			"/org/freedesktop/ModemManager1",
			"org.freedesktop.DBus.ObjectManager",
			"GetManagedObjects",
			{},
			std::string(kMMService));

		auto objects = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(
			reply.get_child(0));

		for (auto i = 0; i != objects.get_n_children(); ++i) {
			try {
				auto object = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(
					objects.get_child(i));

				auto objectPath = GlibVariantCast<Glib::ustring>(object.get_child(0));

				const auto interfaces = GlibVariantCast<
					std::map<
						Glib::ustring,
						std::map<
							Glib::ustring,
							Glib::VariantBase>>>(object.get_child(1));

				CellNetwork currentResult;
				
				for (const auto &[interface, properties] : interfaces) {
					if (interface == "org.freedesktop.ModemManager1.Modem") {
						const auto currentMode = GlibVariantCast<uint>(
							Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(
								properties.at("CurrentModes")).get_child(0));

						switch (currentMode) {
						case MM_MODEM_MODE_2G:
							currentResult.type = CellNetwork::Type::GSM_2G;
							break;
						case MM_MODEM_MODE_3G:
							currentResult.type = CellNetwork::Type::GSM_3G;
							break;
						case MM_MODEM_MODE_4G:
							currentResult.type = CellNetwork::Type::GSM_4G;
							break;
						default:
							currentResult.type = CellNetwork::Type::Unknown;
							break;
						}

						currentResult.rssi = -GlibVariantCast<uint>(
							Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(
								properties.at("SignalQuality")).get_child(0));
					} else if (interface == "org.freedesktop.ModemManager1.Modem.Modem3gpp") {
						currentResult.carrier = QString::fromStdString(
							GlibVariantCast<Glib::ustring>(properties.at("OperatorName")));
					} else if (interface == "org.freedesktop.ModemManager1.Modem.Location") {
						auto reply = connection->call_sync(
							objectPath,
							interface,
							"GetLocation",
							{},
							std::string(kMMService));

						const auto location = QString::fromStdString(
							GlibVariantCast<Glib::ustring>(
								GlibVariantCast<std::map<uint, Glib::VariantBase>>(
									reply.get_child(0)
								).at(MM_MODEM_LOCATION_SOURCE_3GPP_LAC_CI))).split(',');
						
						if (location.size() < 5) {
							throw std::out_of_range("std::out_of_range");
						}

						const auto lac = location[2].toUInt(nullptr, 16);
						const auto tac = location[4].toUInt(nullptr, 16);

						currentResult.mcc = location[0].toUInt(nullptr);
						currentResult.mnc = location[1].toUInt(nullptr);
						currentResult.cellid = location[3].toUInt(nullptr, 16);
						currentResult.lac = lac
							? lac
							: tac;
					}
				}

				result.push_back(currentResult);
			} catch (...) {
			}
		}
	} catch (...) {
	}

	return result;
}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

} // namespace

std::vector<CellNetwork> CellNetworks() {
#ifndef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
	const auto mmResult = MMCellNetworks();
	if (!mmResult.empty()) {
		return mmResult;
	}
#endif // !DESKTOP_APP_DISABLE_DBUS_INTEGRATION

	return {};
}

} // namespace base::Platform
