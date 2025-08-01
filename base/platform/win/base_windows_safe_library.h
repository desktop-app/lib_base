// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include <windows.h>

// We try to keep this module free of external dependencies.

namespace base::Platform {
namespace details {

[[nodiscard]] void *LoadMethodRaw(HINSTANCE library, LPCSTR name, WORD id);
void ReportLoadFailure(HINSTANCE library, LPCSTR name, DWORD id);

} // namespace details

void InitDynamicLibraries();

HINSTANCE SafeLoadLibrary(LPCWSTR name, bool required = false);

template <typename Function>
bool LoadMethod(HINSTANCE library, LPCSTR name, Function &f, WORD id = 0) {
	if (!library) {
		return false;
	} else if (const auto ptr = details::LoadMethodRaw(library, name, id)) {
		f = reinterpret_cast<Function>(ptr);
		return true;
	}
	f = nullptr;
	details::ReportLoadFailure(library, name, id);
	return false;
}

} // namespace base::Platform
