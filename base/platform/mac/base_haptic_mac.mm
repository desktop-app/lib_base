// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/mac/base_haptic_mac.h"

#include "base/integration.h"

#include <Cocoa/Cocoa.h>

namespace base::Platform {

void Haptic() {
	Integration::Instance().enterFromEventLoop([=] {
		[[NSHapticFeedbackManager defaultPerformer]
			performFeedbackPattern:NSHapticFeedbackPatternGeneric
			performanceTime:NSHapticFeedbackPerformanceTimeDrawCompleted];
	});
}

bool IsSwipeBackEnabled() {
	static const auto cached = [] {
		const auto defaults = [NSUserDefaults standardUserDefaults];
		NSNumber *setting = [defaults
			objectForKey:@"AppleEnableSwipeNavigateWithScrolls"];
		return setting ? [setting boolValue] : true;
	}();
	return cached;
}

} // namespace base::Platform
