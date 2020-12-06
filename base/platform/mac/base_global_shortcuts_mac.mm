// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/mac/base_global_shortcuts_mac.h"

#include "base/platform/mac/base_utilities_mac.h"
#include "base/invoke_queued.h"

#include <Carbon/Carbon.h>
#import <IOKit/hidsystem/IOHIDLib.h>

namespace base::Platform {
namespace {

constexpr auto kShortcutLimit = 4;

CFMachPortRef EventPort = nullptr;
CFRunLoopSourceRef EventPortSource = nullptr;
CFRunLoopRef ThreadRunLoop = nullptr;
std::thread Thread;
std::mutex GlobalMutex;
std::vector<not_null<GlobalShortcutManagerMac*>> Managers;
bool Running = false;

[[nodiscard]] bool HasAccess() {
	if (@available(macOS 10.15, *)) {
		// Input Monitoring is required on macOS 10.15 an later.
		// Even if user grants access, restart is required.
		static const auto result = IOHIDCheckAccess(kIOHIDRequestTypeListenEvent);
		return (result == kIOHIDAccessTypeGranted);
	} else if (@available(macOS 10.14, *)) {
		// Accessibility is required on macOS 10.14.
		NSDictionary *const options=@{(__bridge NSString *)kAXTrustedCheckOptionPrompt: @FALSE};
		return AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);
	}
	return true;
}

CGEventRef EventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void*) {
	if (CGEventGetIntegerValueField(event, kCGKeyboardEventAutorepeat)) {
		return event;
	}
	const auto keycode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
	if (keycode == 0xB3) {
		// Some KeyDown+KeyUp sent when quickly pressing and releasing Fn.
		return event;
	}

	const auto flags = CGEventGetFlags(event);
	const auto maybeDown = [&]() -> std::optional<bool> {
		if (type == kCGEventKeyDown) {
			return true;
		} else if (type == kCGEventKeyUp) {
			return false;
		} else if (type != kCGEventFlagsChanged) {
			return std::nullopt;
		}
		switch (keycode) {
		case kVK_CapsLock:
			return (flags & kCGEventFlagMaskAlphaShift) != 0;
		case kVK_Shift:
		case kVK_RightShift:
			return (flags & kCGEventFlagMaskShift) != 0;
		case kVK_Control:
		case kVK_RightControl:
			return (flags & kCGEventFlagMaskControl) != 0;
		case kVK_Option:
		case kVK_RightOption:
			return (flags & kCGEventFlagMaskAlternate) != 0;
		case kVK_Command:
		case kVK_RightCommand:
			return (flags & kCGEventFlagMaskCommand) != 0;
		case kVK_Function:
			return (flags & kCGEventFlagMaskSecondaryFn) != 0;
		default:
			return std::nullopt;
		}
	}();
	if (!maybeDown) {
		return event;
	}
	const auto descriptor = MacKeyDescriptor{
		.keycode = keycode,
	};
	const auto down = *maybeDown;

	std::unique_lock lock{ GlobalMutex };
	for (const auto manager : Managers) {
		manager->schedule(descriptor, down);
	}
	return event;
}

[[nodiscard]] bool Matches(
		const std::vector<MacKeyDescriptor> &sorted,
		const base::flat_set<MacKeyDescriptor> &down) {
	if (sorted.size() > down.size()) {
		return false;
	}
	auto j = begin(down);
	for (const auto descriptor : sorted) {
		while (true) {
			if (*j > descriptor) {
				return false;
			} else if (*j < descriptor) {
				++j;
				if (j == end(down)) {
					return false;
				}
			} else {
				break;
			}
		}
	}
	return true;
}

[[nodiscard]] GlobalShortcut MakeShortcut(
		std::vector<MacKeyDescriptor> descriptors) {
	return std::make_shared<GlobalShortcutValueMac>(std::move(descriptors));
}

} // namespace

std::unique_ptr<GlobalShortcutManager> CreateGlobalShortcutManager() {
	return std::make_unique<GlobalShortcutManagerMac>();
}

GlobalShortcutValueMac::GlobalShortcutValueMac(
	std::vector<MacKeyDescriptor> descriptors)
: _descriptors(std::move(descriptors)) {
	Expects(!_descriptors.empty());
}

QString GlobalShortcutValueMac::toDisplayString() {
	auto result = QStringList();
	result.reserve(_descriptors.size());
	for (const auto descriptor : _descriptors) {
		result.push_back(KeyName(descriptor));
	}
	return result.join(" + ");
}

QByteArray GlobalShortcutValueMac::serialize() {
	static_assert(sizeof(MacKeyDescriptor) == sizeof(uint64));

	const auto size = sizeof(MacKeyDescriptor) * _descriptors.size();
	auto result = QByteArray(size, Qt::Uninitialized);
	memcpy(result.data(), _descriptors.data(), size);
	return result;
}

GlobalShortcutManagerMac::GlobalShortcutManagerMac()
: _valid(HasAccess()) {
	if (!_valid) {
		return;
	} else if (!EventPort) {
		EventPort = CGEventTapCreate(
			kCGHIDEventTap,
			kCGHeadInsertEventTap,
			kCGEventTapOptionListenOnly,
			(CGEventMaskBit(kCGEventKeyDown)
				| CGEventMaskBit(kCGEventKeyUp)
				| CGEventMaskBit(kCGEventFlagsChanged)),
			EventTapCallback,
			nullptr);
		if (!EventPort) {
			return;
		}
		EventPortSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, EventPort, 0);
		if (!EventPortSource) {
			CFMachPortInvalidate(EventPort);
			CFRelease(EventPort);
			EventPort = nullptr;
			return;
		}
		Thread = std::thread([] {
			ThreadRunLoop = CFRunLoopGetCurrent();
			CFRunLoopAddSource(ThreadRunLoop, EventPortSource, kCFRunLoopCommonModes);
			CGEventTapEnable(EventPort, true);
			CFRunLoopRun();
		});
    }

	std::unique_lock lock{ GlobalMutex };
	Managers.push_back(this);
	lock.unlock();
}

GlobalShortcutManagerMac::~GlobalShortcutManagerMac() {
	std::unique_lock lock{ GlobalMutex };
	Managers.erase(ranges::remove(Managers, not_null{ this }), end(Managers));
	const auto stopThread = Managers.empty();
	lock.unlock();

	if (stopThread && EventPort) {
		CFRunLoopStop(ThreadRunLoop);
		Thread.join();

        CFMachPortInvalidate(EventPort);
        CFRelease(EventPort);
        EventPort = nullptr;

        CFRelease(EventPortSource);
        EventPortSource = nullptr;
	}
}

void GlobalShortcutManagerMac::startRecording(
		Fn<void(GlobalShortcut)> progress,
		Fn<void(GlobalShortcut)> done) {
	Expects(done != nullptr);

    if (@available(macOS 10.15, *)) {
		IOHIDRequestAccess(kIOHIDRequestTypeListenEvent);
	}

	_recordingDown.clear();
	_recordingUp.clear();
	_recording = true;
	_recordingProgress = std::move(progress);
	_recordingDone = std::move(done);
}

void GlobalShortcutManagerMac::stopRecording() {
}

void GlobalShortcutManagerMac::startWatching(
		GlobalShortcut shortcut,
		Fn<void(bool pressed)> callback) {
	Expects(shortcut != nullptr);
	Expects(callback != nullptr);

    if (@available(macOS 10.15, *)) {
		IOHIDRequestAccess(kIOHIDRequestTypeListenEvent);
	}

	const auto i = ranges::find(_watchlist, shortcut, &Watch::shortcut);
	if (i != end(_watchlist)) {
		i->callback = std::move(callback);
	} else {
		auto sorted = static_cast<GlobalShortcutValueMac*>(
			shortcut.get())->descriptors();
		std::sort(begin(sorted), end(sorted));
		_watchlist.push_back(Watch{
			std::move(shortcut),
			std::move(sorted),
			std::move(callback)
		});
	}
}

void GlobalShortcutManagerMac::stopWatching(GlobalShortcut shortcut) {
	const auto i = ranges::find(_watchlist, shortcut, &Watch::shortcut);
	if (i != end(_watchlist)) {
		_watchlist.erase(i);
	}
	_pressed.erase(ranges::find(_pressed, shortcut), end(_pressed));
}

GlobalShortcut GlobalShortcutManagerMac::shortcutFromSerialized(
		QByteArray serialized) {
	const auto single = sizeof(MacKeyDescriptor);
	if (serialized.isEmpty() || serialized.size() % single) {
		return nullptr;
	}
	auto count = serialized.size() / single;
	auto list = std::vector<MacKeyDescriptor>(count);
	memcpy(list.data(), serialized.constData(), serialized.size());
	return MakeShortcut(std::move(list));
}

void GlobalShortcutManagerMac::schedule(
		MacKeyDescriptor descriptor,
		bool down) {
	InvokeQueued(this, [=] { process(descriptor, down); });
}

void GlobalShortcutManagerMac::process(
		MacKeyDescriptor descriptor,
		bool down) {
	if (!down) {
		_down.remove(descriptor);
	}
	if (_recording) {
		processRecording(descriptor, down);
		return;
	}
	auto scheduled = std::vector<Fn<void(bool pressed)>>();
	if (down) {
		_down.emplace(descriptor);
		for (const auto &watch : _watchlist) {
			if (watch.sorted.size() > _down.size()
				|| ranges::contains(_pressed, watch.shortcut)) {
				continue;
			} else if (Matches(watch.sorted, _down)) {
				_pressed.push_back(watch.shortcut);
				scheduled.push_back(watch.callback);
			}
		}
	} else {
		_down.remove(descriptor);
		for (auto i = begin(_pressed); i != end(_pressed);) {
			const auto win = static_cast<GlobalShortcutValueMac*>(i->get());
			if (!ranges::contains(win->descriptors(), descriptor)) {
				++i;
			} else {
				const auto j = ranges::find(
					_watchlist,
					*i,
					&Watch::shortcut);
				Assert(j != end(_watchlist));
				scheduled.push_back(j->callback);

				i = _pressed.erase(i);
			}
		}
	}
	for (const auto &callback : scheduled) {
		callback(down);
	}
}

void GlobalShortcutManagerMac::processRecording(
		MacKeyDescriptor descriptor,
		bool down) {
	if (down) {
		processRecordingPress(descriptor);
	} else {
		processRecordingRelease(descriptor);
	}
}

void GlobalShortcutManagerMac::processRecordingPress(
		MacKeyDescriptor descriptor) {
	auto changed = false;
	_recordingUp.remove(descriptor);
	for (const auto descriptor : _recordingUp) {
		const auto i = ranges::remove(_recordingDown, descriptor);
		if (i != end(_recordingDown)) {
			_recordingDown.erase(i, end(_recordingDown));
			changed = true;
		}
	}
	_recordingUp.clear();

	const auto i = std::find(
		begin(_recordingDown),
		end(_recordingDown),
		descriptor);
	if (i == _recordingDown.end()) {
		_recordingDown.push_back(descriptor);
		changed = true;
	}
	if (!changed) {
		return;
	} else if (_recordingDown.size() == kShortcutLimit) {
		finishRecording();
	} else if (const auto onstack = _recordingProgress) {
		onstack(MakeShortcut(_recordingDown));
	}
}

void GlobalShortcutManagerMac::processRecordingRelease(
		MacKeyDescriptor descriptor) {
	const auto i = std::find(
		begin(_recordingDown),
		end(_recordingDown),
		descriptor);
	if (i == end(_recordingDown)) {
		return;
	}
	_recordingUp.emplace(descriptor);
	Assert(_recordingUp.size() <= _recordingDown.size());
	if (_recordingUp.size() == _recordingDown.size()) {
		// All keys are up, we got the shortcut.
		// Some down keys are not up yet.
		finishRecording();
	}
}

void GlobalShortcutManagerMac::finishRecording() {
	Expects(!_recordingDown.empty());

	auto result = MakeShortcut(std::move(_recordingDown));
	_recordingDown.clear();
	_recordingUp.clear();
	_recording = false;
	const auto done = _recordingDone;
	_recordingDone = nullptr;
	_recordingProgress = nullptr;

	done(std::move(result));
}

} // namespace base::Platform
