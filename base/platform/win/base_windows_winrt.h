// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

// class has virtual functions, but destructor is not virtual
#pragma warning(push)
#pragma warning(disable:4265)
#include <winrt/base.h>
#pragma warning(pop)

namespace base::Platform {

[[nodiscard]] bool ResolveWinRT();

} // namespace base::Platform
