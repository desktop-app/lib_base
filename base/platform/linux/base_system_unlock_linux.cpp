// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_system_unlock_linux.h"

namespace base {

rpl::producer<SystemUnlockAvailability> SystemUnlockStatus(
		bool lookupDetails) {
	return rpl::single(SystemUnlockAvailability{ .known = true });
}

void SuggestSystemUnlock(
		not_null<QWidget*> parent,
		const QString &text,
		Fn<void(SystemUnlockResult)> done) {
	done(SystemUnlockResult::Cancelled);
}

} // namespace base
