// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/mac/base_file_utilities_mac.h"

#include "base/platform/mac/base_utilities_mac.h"

#include <QtCore/QFileInfo>

namespace base::Platform {

using namespace ::Platform;

bool ShowInFolder(const QString &filepath) {
	const auto folder = QFileInfo(filepath).absolutePath();
	BOOL result = NO;

	@autoreleasepool {

	result = [[NSWorkspace sharedWorkspace] selectFile:Q2NSString(filepath) inFileViewerRootedAtPath:Q2NSString(folder)];

	}

	return (result != NO);
}

QString CurrentExecutablePath(int argc, char *argv[]) {
	return NS2QString([[NSBundle mainBundle] bundlePath]);
}

} // namespace base::Platform
