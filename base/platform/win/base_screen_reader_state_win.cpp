// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/win/base_screen_reader_state_win.h"

#include <crl/crl_async.h>
#include <crl/crl_on_main.h>

#include <windows.h>
#include <tlhelp32.h>

namespace base::Platform {
namespace {

constexpr auto kCheckPeriod = 5 * crl::time(1000);

[[nodiscard]] bool FindRunningReader() {
	auto pe = PROCESSENTRY32();
	pe.dwSize = sizeof(PROCESSENTRY32);
	const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) {
		return false;
	}
	const auto guard = gsl::finally([&] {
		CloseHandle(snapshot);
	});
	const auto list = std::array{
		L"Narrator.exe",
		L"nvda.exe",
		L"jfw.exe",
		L"Zt.exe",
		L"Snova.exe",
		L"Hal.exe",
		L"wineyes.exe",
		L"FBSA.exe",
		L"CobraController.exe",
	};
	for (auto has = Process32First(snapshot, &pe)
		; has
		; has = Process32Next(snapshot, &pe)) {
		for (const auto &name : list) {
			if (_wcsicmp(pe.szExeFile, name) == 0) {
				return true;
			}
		}
	}
	return false;
}

} // namespace

WinScreenReaderState::WinScreenReaderState()
: _timer([=] { startCheck(); }) {
	QAccessible::installActivationObserver(this);
	if (QAccessible::isActive()) {
		_qaccessibleActive = true;
		startCheck();
	}
}

WinScreenReaderState::~WinScreenReaderState() {
	_timer.cancel();
	++_checkGeneration;
	QAccessible::removeActivationObserver(this);
}

void WinScreenReaderState::startCheck() {
	if (_checking) {
		return;
	}
	_checking = true;
	const auto weak = base::make_weak(this);
	const auto generation = _checkGeneration;
	crl::async([=] {
		const auto found = FindRunningReader();
		crl::on_main(weak, [=] {
			if (generation != _checkGeneration) {
				return;
			}
			if (_qaccessibleActive) {
				_timer.callOnce(kCheckPeriod);
			}
			_checking = false;
			_isActive = found;
		});
	});
}

void WinScreenReaderState::accessibilityActiveChanged(bool active) {
	if (active) {
		_qaccessibleActive = true;
		startCheck();
	} else {
		_qaccessibleActive = false;
		_timer.cancel();
		++_checkGeneration;
		_checking = false;
		_isActive = false;
	}
}

bool WinScreenReaderState::active() const {
	return _isActive.current();
}

rpl::producer<bool> WinScreenReaderState::activeValue() const {
	return _isActive.value();
}

} // namespace base::Platform
