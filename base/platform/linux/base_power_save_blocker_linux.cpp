// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_power_save_blocker_linux.h"

#include "base/platform/base_platform_info.h"
#include "base/timer_rpl.h"

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
#include "base/platform/linux/base_linux_xcb_utilities.h"
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

namespace base::Platform {
namespace {

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
constexpr auto kResetScreenSaverTimeout = 10 * crl::time(1000);

// Due to https://gitlab.freedesktop.org/xorg/xserver/-/issues/363 use the basic reset API
void XCBPreventDisplaySleep(bool prevent) {
	static rpl::lifetime lifetime;
	if (!prevent) {
		lifetime.destroy();
		return;
	} else if (lifetime) {
		return;
	}

	base::timer_each(
		kResetScreenSaverTimeout
	) | rpl::start_with_next([] {
		const auto connection = XCB::GetConnectionFromQt();
		if (!connection) {
			return;
		}
		xcb_force_screen_saver(connection, XCB_SCREEN_SAVER_RESET);
	}, lifetime);
}
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

} // namespace

void BlockPowerSave(
	PowerSaveBlockType type,
	const QString &description,
	QPointer<QWindow> window) {
	switch (type) {
	case PowerSaveBlockType::PreventAppSuspension:
		break;
	case PowerSaveBlockType::PreventDisplaySleep:
#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
		if (::Platform::IsX11()) {
			XCBPreventDisplaySleep(true);
		}
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION
		break;
	}
}

void UnblockPowerSave(PowerSaveBlockType type, QPointer<QWindow> window) {
	switch (type) {
	case PowerSaveBlockType::PreventAppSuspension:
		break;
	case PowerSaveBlockType::PreventDisplaySleep:
#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
		if (::Platform::IsX11()) {
			XCBPreventDisplaySleep(false);
		}
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION
		break;
	}
}

} // namespace base::Platform
