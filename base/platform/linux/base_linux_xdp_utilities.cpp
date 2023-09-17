// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_linux_xdp_utilities.h"

#include "base/platform/base_platform_info.h"

#include <giomm.h>

#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>
#include <private/qgenericunixservices_p.h>

namespace base::Platform::XDP {

Glib::ustring ParentWindowID() {
	return ParentWindowID(QGuiApplication::focusWindow());
}

Glib::ustring ParentWindowID(QWindow *window) {
	if (!window) {
		return {};
	}

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
	if (const auto services = dynamic_cast<QGenericUnixServices*>(
			QGuiApplicationPrivate::platformIntegration()->services())) {
		return services->portalWindowIdentifier(window).toStdString();
	}
#endif // Qt >= 6.5.0

	if (::Platform::IsX11()) {
		std::stringstream result;
		result << "x11:" << std::hex << window->winId();
		return result.str();
	}

	return {};
}

std::optional<Glib::VariantBase> ReadSetting(
		const Glib::ustring &group,
		const Glib::ustring &key) {
	try {
		return Gio::DBus::Connection::get_sync(
			Gio::DBus::BusType::SESSION
		)->call_sync(
			std::string(kObjectPath),
			std::string(kSettingsInterface),
			"Read",
			Glib::create_variant(std::tuple{
				group,
				key,
			}),
			std::string(kService)
		).get_child(
			0
		).get_dynamic<Glib::Variant<Glib::VariantBase>>(
		).get();
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
			Gio::DBus::BusType::SESSION);

		_private->signalId = _private->dbusConnection->signal_subscribe(
			[=](
				const Glib::RefPtr<Gio::DBus::Connection> &connection,
				const Glib::ustring &sender_name,
				const Glib::ustring &object_path,
				const Glib::ustring &interface_name,
				const Glib::ustring &signal_name,
				const Glib::VariantContainerBase &parameters) {
				try {
					const auto group = parameters.get_child(
						0
					).get_dynamic<Glib::ustring>();

					const auto key = parameters.get_child(
						1
					).get_dynamic<Glib::ustring>();

					const auto value = parameters.get_child(
						2
					).get_dynamic<Glib::VariantBase>();

					callback(group, key, value);
				} catch (...) {
				}
			},
			std::string(kService),
			std::string(kSettingsInterface),
			"SettingChanged",
			std::string(kObjectPath));
	} catch (...) {
	}
}

SettingWatcher::~SettingWatcher() {
	if (_private->dbusConnection && _private->signalId != 0) {
		_private->dbusConnection->signal_unsubscribe(_private->signalId);
	}
}

} // namespace base::Platform::XDP
