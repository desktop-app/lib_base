// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/mac/base_battery_saving_mac.h"

#include "base/battery_saving.h"
#include "base/integration.h"

#include <Cocoa/Cocoa.h>
#include <IOKit/ps/IOPSKeys.h>
#include <IOKit/ps/IOPowerSources.h>

@interface LowPowerModeObserver : NSObject {
}

- (id) initWithCallback:(Fn<void()>)callback;
- (void) powerStateChanged:(NSNotification*)aNotification;

@end // @interface LowPowerModeObserver

@implementation LowPowerModeObserver {
	Fn<void()> _callback;
}

- (id) initWithCallback:(Fn<void()>)callback {
	if (self = [super init]) {
		_callback = std::move(callback);
	}
	return self;
}

- (void) powerStateChanged:(NSNotification*)aNotification {
	_callback();
}

@end // @implementation LowPowerModeObserver

namespace base::Platform {
namespace {

class BatterySaving final : public AbstractBatterySaving {
public:
	BatterySaving(Fn<void()> changedCallback);
	~BatterySaving();

	std::optional<bool> enabled() const override;

private:
	static void RunLoopCallback(void *callback) {
		const auto observer = static_cast<LowPowerModeObserver*>(callback);
		[observer powerStateChanged:nil];
	}

	LowPowerModeObserver *_observer = nil;
	CFRunLoopSourceRef _runLoopSource = nullptr;

};

struct BatteryState {
	bool has = false;
	bool draining = false;
	bool low = false;
};

[[nodiscard]] BatteryState DetectBatteryState() {
	CFTypeRef info = IOPSCopyPowerSourcesInfo();
	if (!info) {
		return {};
	}
	const auto infoGuard = gsl::finally([&] { CFRelease(info); });

	CFArrayRef list = IOPSCopyPowerSourcesList(info);
	if (!list) {
		return {};
	}
	const auto listGuard = gsl::finally([&] { CFRelease(list); });

	CFIndex count = CFArrayGetCount(list);

	auto result = BatteryState();
	for (CFIndex i = 0; i < count; ++i) {
		const auto description = CFDictionaryRef(
			IOPSGetPowerSourceDescription(info, CFArrayGetValueAtIndex(list, i)));
		if (!description) {
			continue;
		}
		const auto type = CFStringRef(CFDictionaryGetValue(description, CFSTR(kIOPSTransportTypeKey)));
		if (!type) {
			continue;
		}
		const auto isInternal = (CFStringCompare(type, CFSTR(kIOPSInternalType), 0) == kCFCompareEqualTo);
		if (!isInternal) {
			continue;
		}
		const auto isPresent = CFBooleanRef(CFDictionaryGetValue(description, CFSTR(kIOPSIsPresentKey)));
		if (!isPresent || !CFBooleanGetValue(isPresent)) {
			continue;
		}
		const auto state = CFStringRef(CFDictionaryGetValue(description, CFSTR(kIOPSPowerSourceStateKey)));
		if (!state) {
			continue;
		}
		const auto isDraining = (CFStringCompare(state, CFSTR(kIOPSBatteryPowerValue), 0) == kCFCompareEqualTo);
		result.has = true;
		result.draining = isDraining;

		const auto lowWarnLevel = CFNumberRef(CFDictionaryGetValue(description, CFSTR(kIOPSLowWarnLevelKey)));
		const auto nowCapacity = CFNumberRef(CFDictionaryGetValue(description, CFSTR(kIOPSCurrentCapacityKey)));
		const auto maxCapacity = CFNumberRef(CFDictionaryGetValue(description, CFSTR(kIOPSMaxCapacityKey)));
		if (!lowWarnLevel || !nowCapacity || !maxCapacity) {
			continue;
		}
		auto lowWarnLevelValue = int64_t();
		auto nowCapacityValue = int64_t();
		auto maxCapacityValue = int64_t();
		if (!CFNumberGetValue(lowWarnLevel, kCFNumberSInt64Type, &lowWarnLevelValue)
			|| !CFNumberGetValue(nowCapacity, kCFNumberSInt64Type, &nowCapacityValue)
			|| !CFNumberGetValue(maxCapacity, kCFNumberSInt64Type, &maxCapacityValue)
			|| (lowWarnLevelValue <= 0 || lowWarnLevelValue >= 100)
			|| (nowCapacityValue < 0 || nowCapacityValue > maxCapacityValue || !maxCapacityValue)) {
			continue;
		}
		result.low = (nowCapacityValue / double(maxCapacityValue) <= lowWarnLevelValue / 100.);
	}
	return result;
}

BatterySaving::BatterySaving(Fn<void()> changedCallback) {
	if (!DetectBatteryState().has) {
		return;
	}
	auto wrapped = [copy = std::move(changedCallback)] {
		Integration::Instance().enterFromEventLoop(copy);
	};
	_observer = [[LowPowerModeObserver alloc] initWithCallback:std::move(wrapped)];

	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	if (@available(macOS 12.0, *)) {
		[center
			addObserver: _observer
			selector: @selector(powerStateChanged:)
			name: NSProcessInfoPowerStateDidChangeNotification
			object: nil];
	}
	[center
		addObserver: _observer
		selector: @selector(powerStateChanged:)
		name: NSProcessInfoThermalStateDidChangeNotification
		object: nil];

    _runLoopSource = IOPSNotificationCreateRunLoopSource(
		RunLoopCallback,
		static_cast<void*>(_observer));
	CFRunLoopAddSource(
		CFRunLoopGetCurrent(),
		_runLoopSource,
		kCFRunLoopDefaultMode);
}

BatterySaving::~BatterySaving() {
	if (_runLoopSource) {
		CFRunLoopRemoveSource(
			CFRunLoopGetCurrent(),
			_runLoopSource,
			kCFRunLoopDefaultMode);
		CFRelease(_runLoopSource);
	}
	if (_observer) {
		[_observer release];
	}
}

std::optional<bool> BatterySaving::enabled() const {
	if (!_observer) {
		return std::nullopt;
	}
	NSProcessInfo *info = [NSProcessInfo processInfo];
	if (@available(macOS 12.0, *)) {
		if ([info isLowPowerModeEnabled]) {
			return true;
		}
	}
	const auto state = DetectBatteryState();
	if (!state.has || !state.draining) {
		return false;
	} else if ([info thermalState] == NSProcessInfoThermalStateCritical) {
		return true;
	}
	return state.low;
}

} // namespace

std::unique_ptr<AbstractBatterySaving> CreateBatterySaving(
		Fn<void()> changedCallback) {
	return std::make_unique<BatterySaving>(std::move(changedCallback));
}

} // namespace base::Platform
