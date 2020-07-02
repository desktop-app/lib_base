// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/platform/win/base_windows_h.h"

namespace base {
namespace Platform {

HINSTANCE SafeLoadLibrary(const QString &name);

template <typename Function>
bool LoadMethod(HINSTANCE library, LPCSTR name, Function &func) {
	if (!library) return false;

	func = reinterpret_cast<Function>(GetProcAddress(library, name));
	return (func != nullptr);
}

} // namespace Platform
} // namespace base
