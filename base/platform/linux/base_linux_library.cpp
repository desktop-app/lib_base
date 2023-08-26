// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_linux_library.h"

#include "base/debug_log.h"

namespace base {
namespace Platform {

LibraryHandle LoadLibrary(const char *name, int flags) {
	DEBUG_LOG(("Loading '%1'...").arg(name));
	if (auto lib = LibraryHandle(dlopen(name, RTLD_LAZY | flags))) {
		DEBUG_LOG(("Loaded '%1'!").arg(name));
		return lib;
	}
	LOG(("Could not load '%1'! Error: %2").arg(name).arg(dlerror()));
	return nullptr;
}

void *LoadSymbolGeneric(const LibraryHandle &lib, const char *name) {
	if (!lib) {
		return nullptr;
	} else if (const auto result = dlsym(lib.get(), name)) {
		return result;
	}
	LOG(("Error: failed to load '%1' function: %2").arg(name).arg(dlerror()));
	return nullptr;
}

} // namespace Platform
} // namespace base
