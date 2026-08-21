// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/win/base_battery_saving_win.h"

#include "base/battery_saving.h"
#include "base/integration.h"

#include <windows.h>

namespace base::Platform {
namespace {

constexpr auto kWindowClassName = L"DesktopAppBatterySaving";

// The only thing the window is needed for is being a target handle for
// RegisterPowerSettingNotification, so it is created as a message-only
// window: a child of HWND_MESSAGE is never displayed, never reaches the
// compositor and isn't returned by EnumWindows.
//
// A hidden top-level QWidget was used here before. Even though it was
// never shown, it is a real framed top-level window, and it could end up
// with a live composited visual while WS_VISIBLE stayed clear - leaving
// an unpaintable white rectangle on screen that no ShowWindow(SW_HIDE)
// could take back, because as far as Windows was concerned the window
// already was hidden.
class BatterySaving final : public AbstractBatterySaving {
public:
	explicit BatterySaving(Fn<void()> changedCallback);
	~BatterySaving();

	std::optional<bool> enabled() const override;

private:
	[[nodiscard]] static bool RegisterWindowClass();
	static LRESULT CALLBACK WndProc(
		HWND hwnd,
		UINT message,
		WPARAM wParam,
		LPARAM lParam);

	HWND _hwnd = nullptr;
	HPOWERNOTIFY _notify = nullptr;
	Fn<void()> _changedCallback;

};

bool BatterySaving::RegisterWindowClass() {
	static const auto result = [] {
		auto descriptor = WNDCLASSEXW();
		descriptor.cbSize = sizeof(descriptor);
		descriptor.lpfnWndProc = BatterySaving::WndProc;
		descriptor.hInstance = GetModuleHandleW(nullptr);
		descriptor.lpszClassName = kWindowClassName;
		return RegisterClassExW(&descriptor)
			|| (GetLastError() == ERROR_CLASS_ALREADY_EXISTS);
	}();
	return result;
}

LRESULT CALLBACK BatterySaving::WndProc(
		HWND hwnd,
		UINT message,
		WPARAM wParam,
		LPARAM lParam) {
	if (message == WM_NCCREATE) {
		const auto create = reinterpret_cast<CREATESTRUCTW*>(lParam);
		SetWindowLongPtrW(
			hwnd,
			GWLP_USERDATA,
			reinterpret_cast<LONG_PTR>(create->lpCreateParams));
	} else if (message == WM_POWERBROADCAST) {
		const auto that = reinterpret_cast<BatterySaving*>(
			GetWindowLongPtrW(hwnd, GWLP_USERDATA));
		if (that && that->_changedCallback) {
			Integration::Instance().enterFromEventLoop(
				that->_changedCallback);
		}
		return TRUE;
	}
	return DefWindowProcW(hwnd, message, wParam, lParam);
}

BatterySaving::BatterySaving(Fn<void()> changedCallback)
: _changedCallback(std::move(changedCallback)) {
	if (!_changedCallback || !RegisterWindowClass()) {
		return;
	}
	_hwnd = CreateWindowExW(
		0,
		kWindowClassName,
		nullptr,
		0,
		0,
		0,
		0,
		0,
		HWND_MESSAGE,
		nullptr,
		GetModuleHandleW(nullptr),
		this);
	if (!_hwnd) {
		return;
	}
	_notify = RegisterPowerSettingNotification(
		_hwnd,
		&GUID_POWER_SAVING_STATUS,
		DEVICE_NOTIFY_WINDOW_HANDLE);
}

BatterySaving::~BatterySaving() {
	if (_notify) {
		UnregisterPowerSettingNotification(_notify);
	}
	if (_hwnd) {
		DestroyWindow(_hwnd);
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

} // namespace

std::unique_ptr<AbstractBatterySaving> CreateBatterySaving(
		Fn<void()> changedCallback) {
	return std::make_unique<BatterySaving>(std::move(changedCallback));
}

} // namespace base::Platform
