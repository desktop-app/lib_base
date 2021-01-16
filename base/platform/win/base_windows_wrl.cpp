// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/win/base_windows_wrl.h"

#include "base/platform/win/base_windows_safe_library.h"
#include "base/platform/base_platform_info.h"

namespace base::Platform {
namespace {

using namespace ::Platform;

} // namespace

void InitWRL() {
	static bool Inited = false;
	if (Inited) {
		return;
	}
	Inited = true;

	if (!IsWindows8OrGreater()) {
		return;
	}
	const auto combase = SafeLoadLibrary(u"combase.dll"_q);
	LoadMethod(
		combase,
		"RoGetActivationFactory",
		RoGetActivationFactory);
	LoadMethod(
		combase,
		"WindowsCreateStringReference",
		WindowsCreateStringReference);
	LoadMethod(
		combase,
		"WindowsDeleteString",
		WindowsDeleteString);
}

} // namespace base::Platform
