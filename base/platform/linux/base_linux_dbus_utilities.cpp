// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_linux_dbus_utilities.h"

#include "base/platform/linux/base_linux_glibmm_helper.h"

namespace base::Platform::DBus {

bool NameHasOwner(
		const Glib::RefPtr<Gio::DBus::Connection> &connection,
		const Glib::ustring &name) {
	auto reply = connection->call_sync(
		std::string(kDBusObjectPath),
		std::string(kDBusInterface),
		"NameHasOwner",
		MakeGlibVariant(std::tuple{name}),
		std::string(kDBusService));

	const auto value = GlibVariantCast<bool>(reply.get_child(0));

	return value;
}

std::vector<Glib::ustring> ListActivatableNames(
		const Glib::RefPtr<Gio::DBus::Connection> &connection) {
	auto reply = connection->call_sync(
		std::string(kDBusObjectPath),
		std::string(kDBusInterface),
		"ListActivatableNames",
		{},
		std::string(kDBusService));

	const auto value = GlibVariantCast<std::vector<Glib::ustring>>(
		reply.get_child(0));

	return value;
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
				auto parametersCopy = parameters;

				const auto name = GlibVariantCast<Glib::ustring>(
					parametersCopy.get_child(0));

				const auto oldOwner = GlibVariantCast<Glib::ustring>(
					parametersCopy.get_child(1));

				const auto newOwner = GlibVariantCast<Glib::ustring>(
					parametersCopy.get_child(2));

				if (name != service) {
					return;
				}

				callback(name, oldOwner, newOwner);
			} catch (...) {
			}
		},
		std::string(kDBusService),
		std::string(kDBusInterface),
		"NameOwnerChanged",
		std::string(kDBusObjectPath));
}

} // namespace base::Platform::DBus
