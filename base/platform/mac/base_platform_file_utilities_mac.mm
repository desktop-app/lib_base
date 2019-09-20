// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/mac/base_platform_file_utilities_mac.h"

#include "base/platform/mac/base_platform_utilities_mac.h"

namespace base::Platform {

bool ShowInFolder(const QString &filepath) {
	const auto folder = QFileInfo(filepath).absolutePath();

	@autoreleasepool {

	BOOL result = [[NSWorkspace sharedWorkspace] selectFile:Q2NSString(filepath) inFileViewerRootedAtPath:Q2NSString(folder)];

	}
	return (result != NO);
}

} // namespace base::Platform
