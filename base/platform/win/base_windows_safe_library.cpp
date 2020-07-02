// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/win/base_windows_safe_library.h"

#include "base/flat_map.h"

namespace base {
namespace Platform {

HINSTANCE SafeLoadLibrary(const QString &name) {
	static auto Loaded = base::flat_map<QString, HINSTANCE>();
	static const auto SystemPath = [] {
		WCHAR buffer[MAX_PATH + 1] = { 0 };
		return GetSystemDirectory(buffer, MAX_PATH)
			? QString::fromWCharArray(buffer)
			: QString();
	}();
	static const auto WindowsPath = [] {
		WCHAR buffer[MAX_PATH + 1] = { 0 };
		return GetWindowsDirectory(buffer, MAX_PATH)
			? QString::fromWCharArray(buffer)
			: QString();
	}();
	const auto tryPath = [&](const QString &path) {
		if (!path.isEmpty()) {
			const auto full = (path + '\\' + name).toStdWString();
			if (const auto result = HINSTANCE(LoadLibrary(full.c_str()))) {
				Loaded.emplace(name, result);
				return result;
			}
		}
		return HINSTANCE(nullptr);
	};
	if (const auto i = Loaded.find(name); i != Loaded.end()) {
		return i->second;
	} else if (const auto result1 = tryPath(SystemPath)) {
		return result1;
	} else if (const auto result2 = tryPath(WindowsPath)) {
		return result2;
	}
	Loaded.emplace(name);
	return nullptr;
}

} // namespace Platform
} // namespace base
