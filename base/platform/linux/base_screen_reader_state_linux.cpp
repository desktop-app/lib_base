// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_screen_reader_state_linux.h"

#include "base/platform/linux/base_linux_dbus_utilities.h"
#include "base/platform/base_platform_info.h"
#include "base/integration.h"

#include <QtCore/qtenvironmentvariables.h>

#include <atspistatus/atspistatus.hpp>

#include <memory>
#include <string>
#include <utility>

namespace base::Platform {
namespace {

using namespace gi::repository;
namespace GObject = gi::repository::GObject;

constexpr auto kA11yBusName = "org.a11y.Bus";
constexpr auto kA11yObjectPath = "/org/a11y/bus";
constexpr auto kAlwaysOnEnvironmentVariable
	= "QT_LINUX_ACCESSIBILITY_ALWAYS_ON";
constexpr auto kStatusProxyFlags = Gio::DBusProxyFlags::DO_NOT_AUTO_START_
	| Gio::DBusProxyFlags::DO_NOT_AUTO_START_AT_CONSTRUCTION_;

[[nodiscard]] bool AlwaysOn() {
	return qEnvironmentVariableIsSet(kAlwaysOnEnvironmentVariable);
}

[[nodiscard]] bool UseX11ScreenReaderDetection() {
	return ::Platform::IsX11();
}

[[nodiscard]] AtSpiStatus::Status CreateStatusProxy(
		const Gio::DBusConnection &connection) {
	if (!connection) {
		return {};
	}
	auto result = gi::result<AtSpiStatus::Status>(
		AtSpiStatus::StatusProxy::new_sync(
			connection,
			kStatusProxyFlags,
			kA11yBusName,
			kA11yObjectPath,
			nullptr));
	return result ? *result : AtSpiStatus::Status();
}

[[nodiscard]] Gio::DBusConnection CreateSessionBusConnection() {
	return Gio::bus_get_sync(
		Gio::BusType::SESSION_,
		nullptr);
}

[[nodiscard]] bool NameHasOwner(
		const Gio::DBusConnection &connection,
		const std::string &name) {
	if (!connection) {
		return false;
	}
	const auto result = DBus::NameHasOwner(connection.gobj_(), name);
	return result ? *result : false;
}

// ScreenReaderState is an app-level "real screen reader" signal.
// AT_SPI_BUS_ADDRESS and the X11 AT_SPI_BUS atom only identify an AT-SPI bus,
// so they are intentionally ignored here. QT_LINUX_ACCESSIBILITY_ALWAYS_ON
// is the only environment force-on override because its meaning is explicit.
[[nodiscard]] bool ReadScreenReaderEnabled(
		const AtSpiStatus::Status &status) {
	return status && status.property_screen_reader_enabled().get();
}

void Enqueue(FnMut<void()> update) {
	if (Integration::Exists()) {
		Integration::Instance().enterFromEventLoop(std::move(update));
	} else {
		update();
	}
}

} // namespace

struct LinuxScreenReaderState::Private {
	Gio::DBusConnection connection;
	AtSpiStatus::Status status;
	std::unique_ptr<DBus::ServiceWatcher> serviceWatcher;
};

LinuxScreenReaderState::LinuxScreenReaderState()
: _private(std::make_unique<Private>())
, _useX11ScreenReaderDetection(UseX11ScreenReaderDetection())
, _isActive(
	_useX11ScreenReaderDetection ? AlwaysOn() : QAccessible::isActive()) {
	if (!_useX11ScreenReaderDetection) {
		QAccessible::installActivationObserver(this);
		return;
	} else if (AlwaysOn()) {
		return;
	}

	_private->connection = CreateSessionBusConnection();
	if (!_private->connection) {
		return;
	}

	_private->serviceWatcher = std::make_unique<DBus::ServiceWatcher>(
		_private->connection.gobj_(),
		kA11yBusName,
		[weak = make_weak(this)](
				const std::string &,
				const std::string &,
				const std::string &) {
			Enqueue([weak] {
				if (weak) {
					weak->refreshStatusProxy();
					weak->updateActive();
				}
			});
		});

	refreshStatusProxy();
	updateActive();
}

LinuxScreenReaderState::~LinuxScreenReaderState() {
	if (!_useX11ScreenReaderDetection) {
		QAccessible::removeActivationObserver(this);
	}
}

void LinuxScreenReaderState::accessibilityActiveChanged(bool active) {
	if (!_useX11ScreenReaderDetection) {
		_isActive = active;
	}
}

void LinuxScreenReaderState::refreshStatusProxy() {
	_private->status = {};
	if (!_useX11ScreenReaderDetection
		|| AlwaysOn()
		|| !NameHasOwner(_private->connection, kA11yBusName)) {
		return;
	}

	_private->status = CreateStatusProxy(_private->connection);
	if (!_private->status) {
		return;
	}

	_private->status.property_screen_reader_enabled(
	).signal_notify().connect([weak = make_weak(this)](
			AtSpiStatus::Status,
			GObject::ParamSpec) {
		Enqueue([weak] {
			if (weak) {
				weak->updateActive();
			}
		});
	});
}

void LinuxScreenReaderState::updateActive() {
	if (!_useX11ScreenReaderDetection) {
		_isActive = QAccessible::isActive();
	} else if (AlwaysOn()) {
		_isActive = true;
	} else {
		_isActive = ReadScreenReaderEnabled(_private->status);
	}
}

bool LinuxScreenReaderState::active() const {
	return _isActive.current();
}

rpl::producer<bool> LinuxScreenReaderState::activeValue() const {
	return _isActive.value();
}

} // namespace base::Platform
