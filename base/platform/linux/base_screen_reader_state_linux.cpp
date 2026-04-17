// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_screen_reader_state_linux.h"

#include "base/custom_delete.h"
#include "base/integration.h"

#include <gio/gio.h>

#include <optional>

namespace base::Platform {
namespace {

constexpr auto kA11yBusName = "org.a11y.Bus";
constexpr auto kA11yObjectPath = "/org/a11y/bus";
constexpr auto kA11yStatusInterface = "org.a11y.Status";
constexpr auto kScreenReaderEnabledProperty = "ScreenReaderEnabled";
constexpr auto kAlwaysOnEnvironmentVariable
	= "QT_LINUX_ACCESSIBILITY_ALWAYS_ON";

using ProxyPointer = std::unique_ptr<
	GDBusProxy,
	base::custom_delete<g_object_unref>>;
using VariantPointer = std::unique_ptr<
	GVariant,
	base::custom_delete<g_variant_unref>>;
using ErrorPointer = std::unique_ptr<
	GError,
	base::custom_delete<g_error_free>>;

[[nodiscard]] bool AlwaysOn() {
	return qEnvironmentVariableIsSet(kAlwaysOnEnvironmentVariable);
}

[[nodiscard]] ProxyPointer CreateStatusProxy() {
	GError *error = nullptr;
	auto result = ProxyPointer(g_dbus_proxy_new_for_bus_sync(
		G_BUS_TYPE_SESSION,
		G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START_AT_CONSTRUCTION,
		nullptr,
		kA11yBusName,
		kA11yObjectPath,
		kA11yStatusInterface,
		nullptr,
		&error));
	[[maybe_unused]] const auto errorGuard = ErrorPointer(error);
	return result;
}

[[nodiscard]] std::optional<bool> ReadBooleanVariant(GVariant *value) {
	if (!value) {
		return std::nullopt;
	} else if (g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN)) {
		return static_cast<bool>(g_variant_get_boolean(value));
	} else if (!g_variant_is_of_type(value, G_VARIANT_TYPE("(v)"))) {
		return std::nullopt;
	}

	auto nested = VariantPointer(g_variant_get_child_value(value, 0));
	if (!nested || !g_variant_is_of_type(nested.get(), G_VARIANT_TYPE_BOOLEAN)) {
		return std::nullopt;
	}

	return static_cast<bool>(g_variant_get_boolean(nested.get()));
}

[[nodiscard]] std::optional<bool> ReadCachedBooleanProperty(
		GDBusProxy *proxy,
		const char *property) {
	if (!proxy || !g_dbus_proxy_get_name_owner(proxy)) {
		return std::nullopt;
	}

	auto value = VariantPointer(g_dbus_proxy_get_cached_property(
		proxy,
		property));
	return ReadBooleanVariant(value.get());
}

[[nodiscard]] std::optional<bool> ReadBooleanProperty(
		GDBusProxy *proxy,
		const char *property) {
	if (const auto cached = ReadCachedBooleanProperty(proxy, property)) {
		return cached;
	} else if (!proxy || !g_dbus_proxy_get_name_owner(proxy)) {
		return std::nullopt;
	}

	GError *error = nullptr;
	auto value = VariantPointer(g_dbus_connection_call_sync(
		g_dbus_proxy_get_connection(proxy),
		kA11yBusName,
		kA11yObjectPath,
		"org.freedesktop.DBus.Properties",
		"Get",
		g_variant_new("(ss)", kA11yStatusInterface, property),
		G_VARIANT_TYPE("(v)"),
		G_DBUS_CALL_FLAGS_NO_AUTO_START,
		-1,
		nullptr,
		&error));
	[[maybe_unused]] const auto errorGuard = ErrorPointer(error);
	return ReadBooleanVariant(value.get());
}

[[nodiscard]] bool ReadAccessibilityStatus(GDBusProxy *proxy) {
	return ReadBooleanProperty(
		proxy,
		kScreenReaderEnabledProperty).value_or(false);
}

void EnqueueUpdate(base::weak_ptr<LinuxScreenReaderState> weak) {
	auto update = [weak = std::move(weak)]() mutable {
		if (weak) {
			weak->updateActive();
		}
	};
	if (Integration::Exists()) {
		Integration::Instance().enterFromEventLoop(std::move(update));
	} else {
		update();
	}
}

void ProxyPropertiesChanged(
		GDBusProxy*,
		GVariant*,
		const gchar * const*,
		gpointer userData) {
	EnqueueUpdate(make_weak(static_cast<LinuxScreenReaderState*>(userData)));
}

void ProxyNameOwnerChanged(
		GObject*,
		GParamSpec*,
		gpointer userData) {
	EnqueueUpdate(make_weak(static_cast<LinuxScreenReaderState*>(userData)));
}

} // namespace

struct LinuxScreenReaderState::Private {
	ProxyPointer proxy;
	gulong propertiesChangedHandlerId = 0;
	gulong nameOwnerChangedHandlerId = 0;
};

LinuxScreenReaderState::LinuxScreenReaderState()
: _private(std::make_unique<Private>())
, _isActive(AlwaysOn()) {
	if (AlwaysOn()) {
		return;
	}

	_private->proxy = CreateStatusProxy();
	if (!_private->proxy) {
		return;
	}

	_private->propertiesChangedHandlerId = g_signal_connect(
		_private->proxy.get(),
		"g-properties-changed",
		G_CALLBACK(ProxyPropertiesChanged),
		this);
	_private->nameOwnerChangedHandlerId = g_signal_connect(
		_private->proxy.get(),
		"notify::g-name-owner",
		G_CALLBACK(ProxyNameOwnerChanged),
		this);

	updateActive();
}

LinuxScreenReaderState::~LinuxScreenReaderState() {
	if (!_private->proxy) {
		return;
	}

	if (_private->propertiesChangedHandlerId) {
		g_signal_handler_disconnect(
			_private->proxy.get(),
			_private->propertiesChangedHandlerId);
	}
	if (_private->nameOwnerChangedHandlerId) {
		g_signal_handler_disconnect(
			_private->proxy.get(),
			_private->nameOwnerChangedHandlerId);
	}
}

void LinuxScreenReaderState::updateActive() {
	if (AlwaysOn()) {
		_isActive = true;
		return;
	}

	_isActive = ReadAccessibilityStatus(_private->proxy.get());
}

bool LinuxScreenReaderState::active() const {
	return _isActive.current();
}

rpl::producer<bool> LinuxScreenReaderState::activeValue() const {
	return _isActive.value();
}

} // namespace base::Platform
