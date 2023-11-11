// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/custom_delete.h"

#include <dlfcn.h>
#include <memory>

#define LOAD_LIBRARY_SYMBOL(lib, func) \
	::base::Platform::LoadSymbol(lib, #func, func)

namespace base {
namespace Platform {

using LibraryHandle = std::unique_ptr<void, custom_delete<dlclose>>;

LibraryHandle LoadLibrary(const char *name, int flags = 0);

[[nodiscard]] void *LoadSymbolGeneric(
	const LibraryHandle &lib,
	const char *name);

template <typename Function>
inline bool LoadSymbol(
		const LibraryHandle &lib,
		const char *name,
		Function &func) {
	func = reinterpret_cast<Function>(LoadSymbolGeneric(lib, name));
	return (func != nullptr);
}

} // namespace Platform
} // namespace base
