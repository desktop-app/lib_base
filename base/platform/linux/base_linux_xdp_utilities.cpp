// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_linux_xdp_utilities.h"

#include "base/platform/linux/base_linux_glibmm_helper.h"

namespace base::Platform::XDP {

std::optional<Glib::VariantBase> ReadSetting(
		const Glib::ustring &group,
		const Glib::ustring &key) {
	try {
		const auto connection = Gio::DBus::Connection::get_sync(
			Gio::DBus::BusType::BUS_TYPE_SESSION);

		auto reply = connection->call_sync(
			std::string(kXDPObjectPath),
			std::string(kXDPSettingsInterface),
			"Read",
			MakeGlibVariant(std::tuple{
				group,
				key,
			}),
			std::string(kXDPService));

		return GlibVariantCast<Glib::VariantBase>(
			GlibVariantCast<Glib::VariantBase>(reply.get_child(0)));
	} catch (...) {
	}

	return std::nullopt;
}

class SettingWatcher::Private {
public:
	Glib::RefPtr<Gio::DBus::Connection> dbusConnection;
	uint signalId = 0;
};

SettingWatcher::SettingWatcher(
		Fn<void(
			const Glib::ustring &,
			const Glib::ustring &,
			const Glib::VariantBase &)> callback)
: _private(std::make_unique<Private>()) {
	try {
		_private->dbusConnection = Gio::DBus::Connection::get_sync(
			Gio::DBus::BusType::BUS_TYPE_SESSION);

		_private->signalId = _private->dbusConnection->signal_subscribe(
			[=](
				const Glib::RefPtr<Gio::DBus::Connection> &connection,
				const Glib::ustring &sender_name,
				const Glib::ustring &object_path,
				const Glib::ustring &interface_name,
				const Glib::ustring &signal_name,
				const Glib::VariantContainerBase &parameters) {
				try {
					auto parametersCopy = parameters;

					const auto group = GlibVariantCast<Glib::ustring>(
						parametersCopy.get_child(0));

					const auto key = GlibVariantCast<Glib::ustring>(
						parametersCopy.get_child(1));

					const auto value = GlibVariantCast<Glib::VariantBase>(
						parametersCopy.get_child(2));

					callback(group, key, value);
				} catch (...) {
				}
			},
			std::string(kXDPService),
			std::string(kXDPSettingsInterface),
			"SettingChanged",
			std::string(kXDPObjectPath));
	} catch (...) {
	}
}

SettingWatcher::~SettingWatcher() {
	if (_private->dbusConnection && _private->signalId != 0) {
		_private->dbusConnection->signal_unsubscribe(_private->signalId);
	}
}

} // namespace base::Platform::XDP
