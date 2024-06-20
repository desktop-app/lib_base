// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/win/base_battery_saving_win.h"

#include "base/battery_saving.h"
#include "base/integration.h"

#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QCoreApplication>
#include <QtGui/QWindow>
#include <QtWidgets/QWidget>

#include <windows.h>

namespace base::Platform {
namespace {

class BatterySaving final
	: public AbstractBatterySaving
	, public QAbstractNativeEventFilter {
public:
	BatterySaving(Fn<void()> changedCallback);
	~BatterySaving();

	std::optional<bool> enabled() const override;

private:
	bool nativeEventFilter(
		const QByteArray &eventType,
		void *message,
		native_event_filter_result *result) override;

	QWidget _fake;
	HWND _hwnd = nullptr;
	HPOWERNOTIFY _notify = nullptr;
	Fn<void()> _changedCallback;

};

BatterySaving::BatterySaving(Fn<void()> changedCallback)
: _changedCallback(std::move(changedCallback)) {
	if (!_changedCallback) {
		return;
	}
	_fake.hide();
	_fake.createWinId();
	const auto window = _fake.windowHandle();
	if (!window) {
		return;
	}
	_hwnd = reinterpret_cast<HWND>(window->winId());
	if (!_hwnd) {
		return;
	}
	_notify = RegisterPowerSettingNotification(
		_hwnd,
		&GUID_POWER_SAVING_STATUS,
		DEVICE_NOTIFY_WINDOW_HANDLE);
	if (!_notify) {
		return;
	}
	qApp->installNativeEventFilter(this);
}

BatterySaving::~BatterySaving() {
	if (_notify) {
		qApp->removeNativeEventFilter(this);
		UnregisterPowerSettingNotification(_notify);
	}
}

std::optional<bool> BatterySaving::enabled() const {
	if (_changedCallback && !_notify) {
		return std::nullopt;
	}
	auto status = SYSTEM_POWER_STATUS();
	if (!GetSystemPowerStatus(&status) || (status.BatteryFlag & 128)) {
		return std::nullopt;
	}
	return (status.SystemStatusFlag == 1);
}

bool BatterySaving::nativeEventFilter(
		const QByteArray &eventType,
		void *message,
		native_event_filter_result *result) {
	Expects(_hwnd != nullptr);

	const auto msg = static_cast<MSG*>(message);
	if (msg->hwnd == _hwnd && msg->message == WM_POWERBROADCAST) {
		Integration::Instance().enterFromEventLoop(_changedCallback);
	}
	return false;
}

} // namespace

std::unique_ptr<AbstractBatterySaving> CreateBatterySaving(
		Fn<void()> changedCallback) {
	return std::make_unique<BatterySaving>(std::move(changedCallback));
}

} // namespace base::Platform
