// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

namespace base::Platform {

[[nodiscard]] bool CapsLockStateKnown();
[[nodiscard]] bool CapsLockOn();
[[nodiscard]] rpl::producer<bool> CapsLockOnValue();

} // namespace base::Platform

namespace base {

[[nodiscard]] inline bool CapsLockStateKnown() {
	return Platform::CapsLockStateKnown();
}

[[nodiscard]] inline bool CapsLockOn() {
	return Platform::CapsLockOn();
}

[[nodiscard]] inline rpl::producer<bool> CapsLockOnValue() {
	return Platform::CapsLockOnValue();
}

} // namespace base
