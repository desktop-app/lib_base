// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

namespace base::Platform {

struct WinKeyDescriptor {
	uint32 virtualKeyCode = 0;
	uint32 lParam = 0;
};

inline bool operator<(WinKeyDescriptor a, WinKeyDescriptor b) {
	return (a.virtualKeyCode < b.virtualKeyCode)
		|| ((a.virtualKeyCode == b.virtualKeyCode)
			&& (a.lParam < b.lParam));
}

inline bool operator>(WinKeyDescriptor a, WinKeyDescriptor b) {
	return (b < a);
}

inline bool operator<=(WinKeyDescriptor a, WinKeyDescriptor b) {
	return !(b < a);
}

inline bool operator>=(WinKeyDescriptor a, WinKeyDescriptor b) {
	return !(a < b);
}

inline bool operator==(WinKeyDescriptor a, WinKeyDescriptor b) {
	return (a.virtualKeyCode == b.virtualKeyCode)
		&& (a.lParam == b.lParam);
}

inline bool operator!=(WinKeyDescriptor a, WinKeyDescriptor b) {
	return !(a == b);
}

[[nodiscard]] QString KeyName(WinKeyDescriptor descriptor);

} // namespace base::Platform
