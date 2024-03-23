// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_linux_dbus_utilities.h"

#include <xdgdbus/xdgdbus.hpp>

namespace base::Platform::DBus {
namespace {

using namespace gi::repository;
namespace GObject = gi::repository::GObject;

gi::result<XdgDBus::DBus> MakeInterface(const GDBusConnection *connection) {
	return XdgDBus::DBusProxy::new_sync(
		gi::wrap(connection, gi::transfer_none),
		Gio::DBusProxyFlags::NONE_,
		kService,
		kObjectPath,
		nullptr);
}

auto MakeUnexpected(GLib::Error &&error) {
	return make_unexpected(std::make_unique<GLib::Error>(std::move(error)));
}

} // namespace

Result<bool> NameHasOwner(
		const GDBusConnection *connection,
		const std::string &name) {
	auto interface = MakeInterface(connection);
	if (!interface) {
		return MakeUnexpected(std::move(interface.error()));
	}

	auto result = interface->call_name_has_owner_sync(name);
	if (!result) {
		return MakeUnexpected(std::move(result.error()));
	}

	return std::get<1>(*result);
}

Result<std::vector<std::string>> ListActivatableNames(
		const GDBusConnection *connection) {
	auto interface = MakeInterface(connection);
	if (!interface) {
		return MakeUnexpected(std::move(interface.error()));
	}

	auto result = interface->call_list_activatable_names_sync();
	if (!result) {
		return MakeUnexpected(std::move(result.error()));
	}

	return std::get<1>(*result) | ranges::to<std::vector<std::string>>;
}

Result<StartReply> StartServiceByName(
		const GDBusConnection *connection,
		const std::string &name) {
	auto interface = MakeInterface(connection);
	if (!interface) {
		return MakeUnexpected(std::move(interface.error()));
	}

	auto result = interface->call_start_service_by_name_sync(name, 0);
	if (!result) {
		return MakeUnexpected(std::move(result.error()));
	}

	return StartReply(std::get<1>(*result));
}

void StartServiceByNameAsync(
		const GDBusConnection *connection,
		const std::string &name,
		Fn<void(Fn<Result<StartReply>()>)> callback) {
	auto interface = MakeInterface(connection);
	if (!interface) {
		const auto error = std::make_shared<GLib::Error>(
			std::move(interface.error()));

		callback([=]() -> Result<StartReply> {
			return MakeUnexpected(std::move(*error));
		});

		return;
	}

	interface->call_start_service_by_name(name, 0, [
		=,
		interface = *interface
	](GObject::Object source_object, Gio::AsyncResult res) mutable {
		callback([=]() mutable -> Result<StartReply> {
			auto result = interface.call_start_service_by_name_finish(res);
			if (!result) {
				return MakeUnexpected(std::move(result.error()));
			}

			return StartReply(std::get<1>(*result));
		});
	});
}

struct ServiceWatcher::Private {
	XdgDBus::DBus interface;
};

ServiceWatcher::ServiceWatcher(
	const GDBusConnection *connection,
	const std::string &service,
	Fn<void(
		const std::string &,
		const std::string &,
		const std::string &)> callback)
: _private(std::make_unique<Private>()) {
	XdgDBus::DBusProxy::new_(
		gi::wrap(connection, gi::transfer_none),
		Gio::DBusProxyFlags::NONE_,
		kService,
		kObjectPath,
		nullptr,
		crl::guard(this, [=](GObject::Object, Gio::AsyncResult res) {
			_private->interface = XdgDBus::DBus(
				XdgDBus::DBusProxy::new_finish(res, nullptr));

			if (!_private->interface) {
				return;
			}

			_private->interface.signal_name_owner_changed().connect([=](
					XdgDBus::DBus,
					std::string name,
					std::string oldOwner,
					std::string newOwner) {
				if (name != service) {
					return;
				}

				callback(name, oldOwner, newOwner);
			});
		}));
}

ServiceWatcher::~ServiceWatcher() = default;

} // namespace base::Platform::DBus
