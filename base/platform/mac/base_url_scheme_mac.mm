// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/mac/base_url_scheme_mac.h"

namespace base::Platform {

bool CheckUrlScheme(const UrlSchemeDescriptor &descriptor) {
	return false;
}

void RegisterUrlScheme(const UrlSchemeDescriptor &descriptor) {
}

} // namespace base::Platform
