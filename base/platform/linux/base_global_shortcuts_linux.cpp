// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_global_shortcuts_linux.h"

namespace base::Platform {

std::unique_ptr<GlobalShortcutManager> CreateGlobalShortcutManager() {
	return nullptr;
}

} // namespace base::Platform
