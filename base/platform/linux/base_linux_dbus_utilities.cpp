// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_linux_dbus_utilities.h"

namespace base::Platform::DBus {

bool NameHasOwner(
		const Glib::RefPtr<Gio::DBus::Connection> &connection,
		const Glib::ustring &name) {
	return connection->call_sync(
		kObjectPath,
		kInterface,
		"NameHasOwner",
		Glib::create_variant(std::tuple{name}),
		kService
	).get_child(0).get_dynamic<bool>();
}

std::vector<Glib::ustring> ListActivatableNames(
		const Glib::RefPtr<Gio::DBus::Connection> &connection) {
	return connection->call_sync(
		kObjectPath,
		kInterface,
		"ListActivatableNames",
		{},
		kService
	).get_child(0).get_dynamic<std::vector<Glib::ustring>>();
}

StartReply StartServiceByName(
		const Glib::RefPtr<Gio::DBus::Connection> &connection,
		const Glib::ustring &name) {
	return StartReply(
		connection->call_sync(
			kObjectPath,
			kInterface,
			"StartServiceByName",
			Glib::create_variant(std::tuple{ name, uint(0) }),
			kService
		).get_child(0).get_dynamic<uint>()
	);
}

void StartServiceByNameAsync(
		const Glib::RefPtr<Gio::DBus::Connection> &connection,
		const Glib::ustring &name,
		Fn<void(Fn<StartReply()>)> callback) {
	connection->call(
		kObjectPath,
		kInterface,
		"StartServiceByName",
		Glib::create_variant(std::tuple{ name, uint(0) }),
		[=](const Glib::RefPtr<Gio::AsyncResult> &result) {
			callback([=] {
				return StartReply(
					connection->call_finish(
						result
					).get_child(
						0
					).get_dynamic<uint>()
				);
			});
		},
		kService);
}

uint RegisterServiceWatcher(
		const Glib::RefPtr<Gio::DBus::Connection> &connection,
		const Glib::ustring &service,
		Fn<void(
			const Glib::ustring &,
			const Glib::ustring &,
			const Glib::ustring &)> callback) {
	return connection->signal_subscribe(
		[=](
			const Glib::RefPtr<Gio::DBus::Connection> &connection,
			const Glib::ustring &sender_name,
			const Glib::ustring &object_path,
			const Glib::ustring &interface_name,
			const Glib::ustring &signal_name,
			const Glib::VariantContainerBase &parameters) {
			try {
				const auto name = parameters.get_child(
					0
				).get_dynamic<Glib::ustring>();

				const auto oldOwner =  parameters.get_child(
					1
				).get_dynamic<Glib::ustring>();

				const auto newOwner =  parameters.get_child(
					2
				).get_dynamic<Glib::ustring>();

				if (name != service) {
					return;
				}

				callback(name, oldOwner, newOwner);
			} catch (...) {
			}
		},
		kService,
		kInterface,
		"NameOwnerChanged",
		kObjectPath);
}

} // namespace base::Platform::DBus
