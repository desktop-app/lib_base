// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/mac/base_screen_reader_state_mac.h"

#include "base/integration.h"

#include <Cocoa/Cocoa.h>

@interface VoiceOverObserver : NSObject {
}

- (id) initWithCallback:(Fn<void()>)callback;
- (void) voiceOverStatusChanged:(NSNotification *)notification;

@end // @interface VoiceOverObserver

@implementation VoiceOverObserver {
	Fn<void()> _callback;
}

- (id) initWithCallback:(Fn<void()>)callback {
	if (self = [super init]) {
		_callback = std::move(callback);
	}
	return self;
}

- (void) voiceOverStatusChanged:(NSNotification *)notification {
	_callback();
}

@end // @implementation VoiceOverObserver

namespace base::Platform {
namespace {

VoiceOverObserver *Observer = nil;

} // namespace

MacScreenReaderState::MacScreenReaderState()
: _isActive([[NSWorkspace sharedWorkspace] isVoiceOverEnabled]) {
	auto wrapped = [=] {
		Integration::Instance().enterFromEventLoop([=] {
			_isActive = [[NSWorkspace sharedWorkspace] isVoiceOverEnabled];
		});
	};
	Observer = [[VoiceOverObserver alloc] initWithCallback:std::move(wrapped)];
	[[[NSWorkspace sharedWorkspace] notificationCenter]
		addObserver: Observer
		selector: @selector(voiceOverStatusChanged:)
		name: NSWorkspaceAccessibilityDisplayOptionsDidChangeNotification
		object: nil];
}

MacScreenReaderState::~MacScreenReaderState() {
	if (Observer) {
		[[[NSWorkspace sharedWorkspace] notificationCenter]
			removeObserver: Observer];
		[Observer release];
		Observer = nil;
	}
}

bool MacScreenReaderState::active() const {
	return _isActive.current();
}

rpl::producer<bool> MacScreenReaderState::activeValue() const {
	return _isActive.value();
}

} // namespace base::Platform
