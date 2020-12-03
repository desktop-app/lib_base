// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/win/base_windows_key_codes.h"

#include "base/platform/win/base_windows_h.h"

namespace base::Platform {

QString KeyName(WinKeyDescriptor descriptor) {
	constexpr auto kLimit = 1024;

	WCHAR buffer[kLimit + 1] = { 0 };

	// Remove 25 bit, we want to differentiate between left and right Ctrl-s.
	auto lParam = LONG(descriptor.lParam & ~(1U << 25));

	return GetKeyNameText(lParam, buffer, kLimit)
		? QString::fromWCharArray(buffer)
		: (descriptor.virtualKeyCode == VK_RSHIFT)
		? QString("Right Shift")
		: QString("\\x%1").arg(descriptor.virtualKeyCode, 0, 16);
}

} // namespace base::Platform
