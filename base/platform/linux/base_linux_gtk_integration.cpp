// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_linux_gtk_integration.h"

#ifdef DESKTOP_APP_DISABLE_DBUS_INTEGRATION
#error "GTK integration depends on D-Bus integration."
#endif // DESKTOP_APP_DISABLE_DBUS_INTEGRATION

#include "base/platform/linux/base_linux_gtk_integration_p.h"
#include "base/platform/linux/base_linux_glibmm_helper.h"
#include "base/platform/linux/base_linux_dbus_utilities.h"
#include "base/platform/base_platform_info.h"
#include "base/integration.h"
#include "base/const_string.h"
#include "base/debug_log.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QIcon>

#include <giomm.h>

namespace base {
namespace Platform {
namespace {

using namespace Gtk;

constexpr auto kObjectPath = "/org/desktop_app/GtkIntegration"_cs;
constexpr auto kInterface = "org.desktop_app.GtkIntegration"_cs;
constexpr auto kPropertiesInterface = "org.freedesktop.DBus.Properties"_cs;

constexpr auto kIntrospectionXML = R"INTROSPECTION(<node>
	<interface name='org.desktop_app.GtkIntegration'>
		<method name='Load'>
			<arg type='s' name='allowed-backends' direction='in'/>
		</method>
		<method name='CheckVersion'>
			<arg type='u' name='major' direction='in'/>
			<arg type='u' name='minor' direction='in'/>
			<arg type='u' name='micro' direction='in'/>
			<arg type='b' name='result' direction='out'/>
		</method>
		<method name='GetBoolSetting'>
			<arg type='s' name='property-name' direction='in'/>
			<arg type='b' name='result' direction='out'/>
		</method>
		<method name='GetIntSetting'>
			<arg type='s' name='property-name' direction='in'/>
			<arg type='i' name='result' direction='out'/>
		</method>
		<method name='GetStringSetting'>
			<arg type='s' name='property-name' direction='in'/>
			<arg type='s' name='result' direction='out'/>
		</method>
		<method name='ConnectToSetting'>
			<arg type='s' name='property-name' direction='in'/>
		</method>
		<signal name='SettingChanged'>
			<arg type='s' name='property-name' direction='out'/>
		</signal>
		<property name='Loaded' type='b' access='read'/>
	</interface>
</node>)INTROSPECTION"_cs;

Glib::ustring ServiceNameVar;

void GtkMessageHandler(
		const gchar *log_domain,
		GLogLevelFlags log_level,
		const gchar *message,
		gpointer unused_data) {
	// Silence false-positive Gtk warnings (we are using Xlib to set
	// the WM_TRANSIENT_FOR hint).
	if (message != qstr("GtkDialog mapped without a transient parent. "
		"This is discouraged.")) {
		// For other messages, call the default handler.
		g_log_default_handler(log_domain, log_level, message, unused_data);
	}
}

template <typename T>
std::optional<T> GtkSetting(const QString &propertyName) {
	if (gtk_settings_get_default == nullptr) {
		return std::nullopt;
	}
	T value;
	g_object_get(
		gtk_settings_get_default(),
		propertyName.toUtf8().constData(),
		&value,
		nullptr);
	return value;
}

} // namespace

class GtkIntegration::Private {
public:
	Private()
	: dbusConnection([] {
		try {
			return Gio::DBus::Connection::get_sync(
				Gio::DBus::BusType::BUS_TYPE_SESSION);
		} catch (...) {
			return Glib::RefPtr<Gio::DBus::Connection>();
		}
	}())
	, interfaceVTable(
		sigc::mem_fun(this, &Private::handleMethodCall),
		sigc::mem_fun(this, &Private::handleGetProperty)) {
	}

	bool setupBase(QLibrary &lib, const QString &allowedBackends);

	void handleMethodCall(
		const Glib::RefPtr<Gio::DBus::Connection> &connection,
		const Glib::ustring &sender,
		const Glib::ustring &object_path,
		const Glib::ustring &interface_name,
		const Glib::ustring &method_name,
		const Glib::VariantContainerBase &parameters,
		const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation);

	void handleGetProperty(
		Glib::VariantBase &property,
		const Glib::RefPtr<Gio::DBus::Connection> &connection,
		const Glib::ustring &sender,
		const Glib::ustring &object_path,
		const Glib::ustring &interface_name,
		const Glib::ustring &property_name);

	const Glib::RefPtr<Gio::DBus::Connection> dbusConnection;
	const Gio::DBus::InterfaceVTable interfaceVTable;
	Glib::RefPtr<Gio::DBus::NodeInfo> introspectionData;
	Glib::ustring parentDBusName;
	QStringList connectedSettings;
	bool loaded = false;
	bool triedToInit = false;
	bool remoting = true;
	uint registerId = 0;
	uint parentServiceWatcherId = 0;
};

bool GtkIntegration::Private::setupBase(
		QLibrary &lib,
		const QString &allowedBackends) {
	if (!LOAD_GTK_SYMBOL(lib, gtk_init_check)) return false;

	if (LOAD_GTK_SYMBOL(lib, gdk_set_allowed_backends)
		&& !allowedBackends.isEmpty()) {
		// We work only with Wayland and X11 GDK backends.
		// Otherwise we get segfault in Ubuntu 17.04 in gtk_init_check() call.
		// See https://github.com/telegramdesktop/tdesktop/issues/3176
		// See https://github.com/telegramdesktop/tdesktop/issues/3162
		gdk_set_allowed_backends(allowedBackends.toUtf8().constData());
	}

	triedToInit = true;
	if (!gtk_init_check(0, 0)) {
		gtk_init_check = nullptr;
		return false;
	}

	// Use our custom log handler.
	g_log_set_handler("Gtk", G_LOG_LEVEL_MESSAGE, GtkMessageHandler, nullptr);

	return true;
}

void GtkIntegration::Private::handleMethodCall(
		const Glib::RefPtr<Gio::DBus::Connection> &connection,
		const Glib::ustring &sender,
		const Glib::ustring &object_path,
		const Glib::ustring &interface_name,
		const Glib::ustring &method_name,
		const Glib::VariantContainerBase &parameters,
		const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) {
	if (sender != parentDBusName) {
		Gio::DBus::Error error(
			Gio::DBus::Error::ACCESS_DENIED,
			"Access denied.");

		invocation->return_error(error);
		return;
	}

	try {
		const auto integration = Instance();
		if (!integration) {
			throw std::exception();
		}

		auto parametersCopy = parameters;

		if (method_name == "Load") {
			const auto allowedBackends = GlibVariantCast<Glib::ustring>(
				parametersCopy.get_child(0));

			integration->load(QString::fromStdString(allowedBackends));
			invocation->return_value({});
			return;
		} else if (method_name == "CheckVersion") {
			const auto major = GlibVariantCast<uint>(
				parametersCopy.get_child(0));

			const auto minor = GlibVariantCast<uint>(
				parametersCopy.get_child(1));

			const auto micro = GlibVariantCast<uint>(
				parametersCopy.get_child(2));

			const auto result = integration->checkVersion(
				major,
				minor,
				micro);

			invocation->return_value(
				Glib::VariantContainerBase::create_tuple(
					Glib::Variant<bool>::create(result)));

			return;
		} else if (method_name == "GetBoolSetting") {
			const auto propertyName = GlibVariantCast<Glib::ustring>(
				parametersCopy.get_child(0));

			const auto result = integration->getBoolSetting(
				QString::fromStdString(propertyName));

			if (result.has_value()) {
				invocation->return_value(
					Glib::VariantContainerBase::create_tuple(
						Glib::Variant<bool>::create(*result)));

				return;
			}
		} else if (method_name == "GetIntSetting") {
			const auto propertyName = GlibVariantCast<Glib::ustring>(
				parametersCopy.get_child(0));

			const auto result = integration->getIntSetting(
				QString::fromStdString(propertyName));

			if (result.has_value()) {
				invocation->return_value(
					Glib::VariantContainerBase::create_tuple(
						Glib::Variant<int>::create(*result)));

				return;
			}
		} else if (method_name == "GetStringSetting") {
			const auto propertyName = GlibVariantCast<Glib::ustring>(
				parametersCopy.get_child(0));

			const auto result = integration->getStringSetting(
				QString::fromStdString(propertyName));

			if (result.has_value()) {
				invocation->return_value(
					Glib::VariantContainerBase::create_tuple(
						Glib::Variant<Glib::ustring>::create(
							result->toStdString())));

				return;
			}
		} else if (method_name == "ConnectToSetting") {
			const auto propertyName = QString::fromStdString(
				GlibVariantCast<Glib::ustring>(
					parametersCopy.get_child(0)));

			if (connectedSettings.contains(propertyName)) {
				invocation->return_value({});
				return;
			}

			integration->connectToSetting(propertyName, [=] {
				try {
					connection->emit_signal(
						std::string(kObjectPath),
						std::string(kInterface),
						"SettingChanged",
						parentDBusName,
						MakeGlibVariant(std::tuple{
							Glib::ustring(propertyName.toStdString()),
						}));
				} catch (...) {
				}
			});

			connectedSettings << propertyName;
			invocation->return_value({});
			return;
		}
	} catch (...) {
	}

	Gio::DBus::Error error(
		Gio::DBus::Error::UNKNOWN_METHOD,
		"Method does not exist.");

	invocation->return_error(error);
}

void GtkIntegration::Private::handleGetProperty(
		Glib::VariantBase &property,
		const Glib::RefPtr<Gio::DBus::Connection> &connection,
		const Glib::ustring &sender,
		const Glib::ustring &object_path,
		const Glib::ustring &interface_name,
		const Glib::ustring &property_name) {
	if (sender != parentDBusName) {
		throw Gio::DBus::Error(
			Gio::DBus::Error::ACCESS_DENIED,
			"Access denied.");
	}

	if (property_name == "Loaded") {
		property = Glib::Variant<bool>::create([] {
			if (const auto integration = Instance()) {
				return integration->loaded();
			}
			return false;
		}());
		return;
	}

	throw Gio::DBus::Error(
		Gio::DBus::Error::NO_REPLY,
		"No reply.");
}

GtkIntegration::GtkIntegration()
: _private(std::make_unique<Private>()) {
}

GtkIntegration::~GtkIntegration() {
	if (_private->dbusConnection) {
		if (_private->parentServiceWatcherId != 0) {
			_private->dbusConnection->signal_unsubscribe(
				_private->parentServiceWatcherId);
		}

		if (_private->registerId != 0) {
			_private->dbusConnection->unregister_object(
				_private->registerId);
		}
	}
}

GtkIntegration *GtkIntegration::Instance() {
	static GtkIntegration instance;
	return &instance;
}

void GtkIntegration::load(const QString &allowedBackends, bool force) {
	Expects(!loaded());

	if (force) {
		_private->remoting = false;
	}

	if (_private->remoting) {
		if (!_private->dbusConnection) {
			return;
		}

		try {
			auto reply = _private->dbusConnection->call_sync(
				std::string(kObjectPath),
				std::string(kInterface),
				"Load",
				MakeGlibVariant(std::tuple{
					Glib::ustring(allowedBackends.toStdString()),
				}),
				ServiceNameVar);
		} catch (...) {
		}

		return;
	}

	if (LoadGtkLibrary(_lib, "gtk-3", 0)) {
		_private->loaded = _private->setupBase(_lib, allowedBackends);
	}
	if (!_private->loaded
		&& !_private->triedToInit
		&& LoadGtkLibrary(_lib, "gtk-x11-2.0", 0)) {
		_private->loaded = _private->setupBase(_lib, allowedBackends);
	}

	if (_private->loaded) {
		LOAD_GTK_SYMBOL(_lib, gtk_check_version);
		LOAD_GTK_SYMBOL(_lib, gtk_settings_get_default);
	}
}

int GtkIntegration::exec(const QString &parentDBusName) {
	_private->remoting = false;
	_private->parentDBusName = parentDBusName.toStdString();

	_private->introspectionData = Gio::DBus::NodeInfo::create_for_xml(
		std::string(kIntrospectionXML));

	_private->registerId = _private->dbusConnection->register_object(
		std::string(kObjectPath),
		_private->introspectionData->lookup_interface(),
		_private->interfaceVTable);

	const auto app = Gio::Application::create(ServiceNameVar);
	app->hold();
	_private->parentServiceWatcherId = DBus::RegisterServiceWatcher(
		_private->dbusConnection,
		parentDBusName.toStdString(),
		[=](
			const Glib::ustring &service,
			const Glib::ustring &oldOwner,
			const Glib::ustring &newOwner) {
			if (!newOwner.empty()) {
				return;
			}
			app->quit();
		});
	return app->run(0, nullptr);
}

bool GtkIntegration::loaded() const {
	if (_private->remoting) {
		if (!_private->dbusConnection || ServiceNameVar.empty()) {
			return false;
		}

		try {
			auto reply = _private->dbusConnection->call_sync(
				std::string(kObjectPath),
				std::string(kPropertiesInterface),
				"Get",
				MakeGlibVariant(std::tuple{
					Glib::ustring(std::string(kInterface)),
					Glib::ustring("Loaded"),
				}),
				ServiceNameVar);

			return GlibVariantCast<bool>(
				GlibVariantCast<Glib::VariantBase>(
					reply.get_child(0)));
		} catch (...) {
		}

		return false;
	}

	return _private->loaded;
}

QString GtkIntegration::ServiceName() {
	return QString::fromStdString(ServiceNameVar);
}

void GtkIntegration::SetServiceName(const QString &serviceName) {
	ServiceNameVar = serviceName.toStdString();
}

bool GtkIntegration::checkVersion(uint major, uint minor, uint micro) const {
	if (_private->remoting) {
		if (!_private->dbusConnection) {
			return false;
		}

		try {
			auto reply = _private->dbusConnection->call_sync(
				std::string(kObjectPath),
				std::string(kInterface),
				"CheckVersion",
				MakeGlibVariant(std::tuple{
					major,
					minor,
					micro,
				}),
				ServiceNameVar);

			return GlibVariantCast<bool>(reply.get_child(0));
		} catch (...) {
		}

		return false;
	}

	return (gtk_check_version != nullptr)
		? !gtk_check_version(major, minor, micro)
		: false;
}

std::optional<bool> GtkIntegration::getBoolSetting(
		const QString &propertyName) const {
	if (_private->remoting) {
		if (!_private->dbusConnection) {
			return std::nullopt;
		}

		try {
			auto reply = _private->dbusConnection->call_sync(
				std::string(kObjectPath),
				std::string(kInterface),
				"GetBoolSetting",
				MakeGlibVariant(std::tuple{
					Glib::ustring(propertyName.toStdString()),
				}),
				ServiceNameVar);

			const auto value = GlibVariantCast<bool>(reply.get_child(0));

			DEBUG_LOG(("Getting GTK setting, %1: %2").arg(
				propertyName,
				value ? "[TRUE]" : "[FALSE]"));

			return value;
		} catch (...) {
		}

		return std::nullopt;
	}

	const auto value = GtkSetting<gboolean>(propertyName);
	if (!value.has_value()) {
		return std::nullopt;
	}
	return *value;
}

std::optional<int> GtkIntegration::getIntSetting(
		const QString &propertyName) const {
	if (_private->remoting) {
		if (!_private->dbusConnection) {
			return std::nullopt;
		}

		try {
			auto reply = _private->dbusConnection->call_sync(
				std::string(kObjectPath),
				std::string(kInterface),
				"GetIntSetting",
				MakeGlibVariant(std::tuple{
					Glib::ustring(propertyName.toStdString()),
				}),
				ServiceNameVar);

			const auto value = GlibVariantCast<int>(reply.get_child(0));

			DEBUG_LOG(("Getting GTK setting, %1: %2")
				.arg(propertyName)
				.arg(value));

			return value;
		} catch (...) {
		}

		return std::nullopt;
	}

	return GtkSetting<gint>(propertyName);
}

std::optional<QString> GtkIntegration::getStringSetting(
		const QString &propertyName) const {
	if (_private->remoting) {
		if (!_private->dbusConnection) {
			return std::nullopt;
		}

		try {
			auto reply = _private->dbusConnection->call_sync(
				std::string(kObjectPath),
				std::string(kInterface),
				"GetStringSetting",
				MakeGlibVariant(std::tuple{
					Glib::ustring(propertyName.toStdString()),
				}),
				ServiceNameVar);

			const auto value = QString::fromStdString(
				GlibVariantCast<Glib::ustring>(reply.get_child(0)));

			DEBUG_LOG(("Getting GTK setting, %1: '%2'").arg(
				propertyName,
				value));

			return value;
		} catch (...) {
		}

		return std::nullopt;
	}

	const auto value = GtkSetting<gchararray>(propertyName);
	if (!value.has_value()) {
		return std::nullopt;
	}

	const auto guard = gsl::finally([&] {
		g_free(*value);
	});

	return QString::fromUtf8(*value);
}

void GtkIntegration::connectToSetting(
		const QString &propertyName,
		Fn<void()> callback) {
	if (_private->remoting) {
		if (!_private->dbusConnection) {
			return;
		}

		try {
			_private->dbusConnection->call(
				std::string(kObjectPath),
				std::string(kInterface),
				"ConnectToSetting",
				MakeGlibVariant(std::tuple{
					Glib::ustring(propertyName.toStdString()),
				}),
				{},
				ServiceNameVar);

			// doesn't emit subscriptions subscribed with
			// crl::async otherwise for some reason...
			crl::on_main([=] {
				_private->dbusConnection->signal_subscribe(
					[=](
						const Glib::RefPtr<Gio::DBus::Connection> &connection,
						const Glib::ustring &sender_name,
						const Glib::ustring &object_path,
						const Glib::ustring &interface_name,
						const Glib::ustring &signal_name,
						Glib::VariantContainerBase parameters) {
						try {
							auto parametersCopy = parameters;

							const auto receivedPropertyName = QString::fromStdString(
								GlibVariantCast<Glib::ustring>(
									parametersCopy.get_child(0)));

							if (propertyName == receivedPropertyName) {
								callback();
							}
						} catch (...) {
						}
					},
					ServiceNameVar,
					std::string(kInterface),
					"SettingChanged",
					std::string(kObjectPath));
			});

			// resubscribe on service restart,
			// doesn't work on non-main thread as well for some reason
			crl::on_main([=] {
				if (!_private->connectedSettings.contains(propertyName)) {
					DBus::RegisterServiceWatcher(
						_private->dbusConnection,
						ServiceNameVar,
						[=, connection = _private->dbusConnection](
							const Glib::ustring &service,
							const Glib::ustring &oldOwner,
							const Glib::ustring &newOwner) {
							if (newOwner.empty()) {
								return;
							}

							connection->call(
								std::string(kObjectPath),
								std::string(kInterface),
								"ConnectToSetting",
								MakeGlibVariant(std::tuple{
									Glib::ustring(propertyName.toStdString()),
								}),
								{},
								service);
						});

					_private->connectedSettings << propertyName;
				}
			});
		} catch (...) {
		}

		return;
	}

	if (gtk_settings_get_default == nullptr) {
		return;
	}

	auto callbackCopy = new Fn<void()>(callback);

	g_signal_connect_swapped(
		gtk_settings_get_default(),
		("notify::" + propertyName).toUtf8().constData(),
		G_CALLBACK(+[](Fn<void()> *callback) {
			(*callback)();
		}),
		callbackCopy);
}

} // namespace Platform
} // namespace base
