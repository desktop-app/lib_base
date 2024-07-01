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

[[nodiscard]] bool Available(LAContext *context, LAPolicy policy) {
	NSError *error = nil;
	return [context canEvaluatePolicy:policy error:&error];
}

[[nodiscard]] SystemUnlockAvailability Available(bool lookupDetails) {
	LAContext *context = [[LAContext alloc] init];

	auto result = SystemUnlockAvailability{
		.known = true,
		.available = Available(
			context,
			LAPolicyDeviceOwnerAuthentication),
		.withBiometrics = lookupDetails && Available(
			context,
			LAPolicyDeviceOwnerAuthenticationWithBiometrics),
	};
	if (lookupDetails) {
		if (@available(macOS 10.15, *)) {
			result.withCompanion = Available(
				context,
				LAPolicyDeviceOwnerAuthenticationWithWatch);
		}
	}
	[context release];

	return result;
}

} // namespace

rpl::producer<SystemUnlockAvailability> SystemUnlockStatus(
		bool lookupDetails) {
	static auto result = rpl::variable<SystemUnlockAvailability>();

	auto refreshed = Available(lookupDetails);
	if (!lookupDetails) {
		const auto now = result.current();
		refreshed.withBiometrics = now.available && now.withBiometrics;
		refreshed.withCompanion = now.available && now.withCompanion;
	}
	result = refreshed;

	return result.value();
}

void SuggestSystemUnlock(
		not_null<QWidget*> parent,
		const QString &text,
		Fn<void(SystemUnlockResult)> done) {
	LAContext *context = [[LAContext alloc] init];
	if (Available(context, LAPolicyDeviceOwnerAuthentication)) {
		[context
			evaluatePolicy:LAPolicyDeviceOwnerAuthentication
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
