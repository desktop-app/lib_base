// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/platform/win/base_windows_h.h"

#define LOAD_LIBRARY_SYMBOL(lib, func) \
	::base::Platform::LoadMethod(lib, #func, func)

namespace base::Platform {

HINSTANCE SafeLoadLibrary(const QString &name);

template <typename Function>
bool LoadMethod(HINSTANCE library, LPCSTR name, Function &func, WORD id = 0) {
	if (!library) return false;

	auto result = GetProcAddress(library, name);
	if (!result && id) {
		result = GetProcAddress(library, MAKEINTRESOURCEA(id));
	}
	func = reinterpret_cast<Function>(result);
	return (func != nullptr);
}

} // namespace base::Platform
