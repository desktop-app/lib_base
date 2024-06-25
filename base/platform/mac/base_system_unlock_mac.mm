// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/mac/base_system_unlock_mac.h"

#include "base/platform/mac/base_utilities_mac.h"

#include <Foundation/Foundation.h>
#include <LocalAuthentication/LocalAuthentication.h>

namespace base {
namespace {

[[nodiscard]] bool Available(LAContext *context) {
	NSError *error = nil;
	return [context
		canEvaluatePolicy:LAPolicyDeviceOwnerAuthenticationWithBiometrics
		error:&error];
}

[[nodiscard]] bool Available() {
	LAContext *context = [[LAContext alloc] init];

	const auto result = Available(context);

	[context release];

	return result;
}

} // namespace

rpl::producer<SystemUnlockAvailability> SystemUnlockStatus() {
	static auto result = rpl::variable<SystemUnlockAvailability>(
		SystemUnlockAvailability::Unknown);

	result = Available()
		? SystemUnlockAvailability::Available
		: SystemUnlockAvailability::Unavailable;

	return result.value();
}

void SuggestSystemUnlock(
		not_null<QWidget*> parent,
		const QString &text,
		Fn<void(SystemUnlockResult)> done) {
	LAContext *context = [[LAContext alloc] init];
	if (Available(context)) {
		[context
			evaluatePolicy:LAPolicyDeviceOwnerAuthenticationWithBiometrics
			localizedReason:Platform::Q2NSString(text)
			reply:^(BOOL success, NSError *error) {
				const auto code = int(error.code);
				if (success) {
					done(SystemUnlockResult::Success);
				} else if (error.code == LAErrorTouchIDLockout) {
					done(SystemUnlockResult::FloodError);
				} else {
					done(SystemUnlockResult::Cancelled);
				}
			}];
	} else {
		done(SystemUnlockResult::Cancelled);
	}
}

} // namespace base
