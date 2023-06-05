// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_battery_saving_linux.h"

#include "base/battery_saving.h"
#include "base/integration.h"

#include <QtCore/QDir>
#include <gio/gio.h>
#include <dlfcn.h>

namespace base::Platform {
namespace {

class BatterySaving final : public AbstractBatterySaving {
public:
	BatterySaving(Fn<void()> changedCallback);
	~BatterySaving();

	std::optional<bool> enabled() const override;

private:
	GPowerProfileMonitor *_monitor = nullptr;
	gulong _handlerId = 0;
	Fn<void()> _changedCallback;

};

BatterySaving::BatterySaving(Fn<void()> changedCallback)
: _changedCallback(std::move(changedCallback)) {
	// Detect battery
	if (QDir(u"/sys/class/power_supply"_q).isEmpty()) {
		return;
	}

	// glib 2.70+, we keep glib 2.56+ compatibility
	static const auto dup_default = [] {
		// reset dlerror after dlsym call
		const auto guard = gsl::finally([] { dlerror(); });
		using T = decltype(&g_power_profile_monitor_dup_default);
		return reinterpret_cast<T>(
			dlsym(RTLD_DEFAULT, "g_power_profile_monitor_dup_default"));
	}();

	if (!dup_default) {
		return;
	}

	_monitor = dup_default();

	if (_changedCallback) {
		_handlerId = g_signal_connect_swapped(
			_monitor,
			"notify::power-saver-enabled",
			G_CALLBACK(+[](BatterySaving *instance) {
				Integration::Instance().enterFromEventLoop([&] {
					instance->_changedCallback();
				});
			}), this);
	}
}

BatterySaving::~BatterySaving() {
	if (_monitor) {
		if (_handlerId) {
			g_signal_handler_disconnect(_monitor, _handlerId);
		}

		g_object_unref(_monitor);
	}
}

std::optional<bool> BatterySaving::enabled() const {
	if (!_monitor) {
		return std::nullopt;
	}

	// glib 2.70+, we keep glib 2.40+ compatibility
	static const auto get_power_saver_enabled = [] {
		// reset dlerror after dlsym call
		const auto guard = gsl::finally([] { dlerror(); });
		using T = decltype(&g_power_profile_monitor_get_power_saver_enabled);
		return reinterpret_cast<T>(
			dlsym(
				RTLD_DEFAULT,
				"g_power_profile_monitor_get_power_saver_enabled"));
	}();

	if (!get_power_saver_enabled) {
		return std::nullopt;
	}

	return get_power_saver_enabled(_monitor);
}

} // namespace

std::unique_ptr<AbstractBatterySaving> CreateBatterySaving(
		Fn<void()> changedCallback) {
	return std::make_unique<BatterySaving>(std::move(changedCallback));
}

} // namespace base::Platform
