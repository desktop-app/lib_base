// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

namespace base::Platform {

struct MacKeyDescriptor {
	int64 keycode = 0;
};

inline bool operator<(MacKeyDescriptor a, MacKeyDescriptor b) {
	return (a.keycode < b.keycode);
}

inline bool operator>(MacKeyDescriptor a, MacKeyDescriptor b) {
	return (b < a);
}

inline bool operator<=(MacKeyDescriptor a, MacKeyDescriptor b) {
	return !(b < a);
}

inline bool operator>=(MacKeyDescriptor a, MacKeyDescriptor b) {
	return !(a < b);
}

inline bool operator==(MacKeyDescriptor a, MacKeyDescriptor b) {
	return (a.keycode == b.keycode);
}

inline bool operator!=(MacKeyDescriptor a, MacKeyDescriptor b) {
	return !(a == b);
}

[[nodiscard]] QString KeyName(MacKeyDescriptor descriptor);

} // namespace base::Platform
