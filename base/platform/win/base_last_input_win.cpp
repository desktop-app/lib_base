// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/win/base_last_input_win.h"

#include "base/platform/win/base_windows_h.h"

namespace base::Platform {

std::optional<crl::time> LastUserInputTime() {
	auto lii = LASTINPUTINFO{ 0 };
	lii.cbSize = sizeof(LASTINPUTINFO);
	return GetLastInputInfo(&lii)
		? std::make_optional(crl::now() + lii.dwTime - GetTickCount())
		: std::nullopt;
}

} // namespace base::Platform
