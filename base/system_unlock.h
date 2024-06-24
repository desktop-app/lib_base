// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

class QWidget;
class QString;

namespace base {

enum class SystemUnlockAvailability {
	Unknown,
	Available,
	Unavailable,
};

[[nodiscard]] rpl::producer<SystemUnlockAvailability> SystemUnlockStatus();

enum class SystemUnlockResult {
	Success,
	Cancelled,
	FloodError,
};

void SuggestSystemUnlock(
	not_null<QWidget*> parent,
	const QString &text,
	Fn<void(SystemUnlockResult)> done);

} // namespace base
