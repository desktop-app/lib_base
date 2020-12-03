// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/win/base_global_shortcuts_win.h"

#include "base/platform/win/base_windows_key_codes.h"
#include "base/platform/win/base_windows_h.h"
#include "base/invoke_queued.h"

namespace base::Platform {
namespace {

constexpr auto kShortcutLimit = 4;

HHOOK GlobalHook = nullptr;
HANDLE ThreadHandle = nullptr;
HANDLE ThreadEvent = nullptr;
DWORD ThreadId = 0;
std::mutex GlobalMutex;
std::vector<not_null<GlobalShortcutManagerWin*>> Managers;
bool Running = false;

LRESULT CALLBACK LowLevelKeyboardProc(
		_In_ int nCode,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam) {
	if (nCode == HC_ACTION) {
		const auto press = (PKBDLLHOOKSTRUCT)lParam;
		const auto repeatCount = uint32(0);
		const auto extendedBit = ((press->flags & LLKHF_EXTENDED) != 0);
		const auto contextBit = ((press->flags & LLKHF_ALTDOWN) != 0);
		const auto transitionState = ((press->flags & LLKHF_UP) != 0);
		const auto lParam = (repeatCount & 0x0000FFFFU)
			| ((uint32(press->scanCode) & 0xFFU) << 16)
			| (extendedBit ? (uint32(KF_EXTENDED) << 16) : 0);
			//| (contextBit ? (uint32(KF_ALTDOWN) << 16) : 0); // Alt pressed.
			//| (transitionState ? (uint32(KF_UP) << 16) : 0); // Is pressed.
		const auto descriptor = WinKeyDescriptor{
			.virtualKeyCode = press->vkCode,
			.lParam = lParam,
		};
		const auto down = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);

		std::unique_lock lock{ GlobalMutex };
		for (const auto manager : Managers) {
			manager->schedule(descriptor, down);
		}
	}
	return CallNextHookEx(GlobalHook, nCode, wParam, lParam);
}

DWORD WINAPI RunThread(LPVOID) {
	auto message = MSG();

	// Force message loop creation.
	PeekMessage(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
	SetEvent(ThreadEvent);

	GlobalHook = SetWindowsHookEx(
		WH_KEYBOARD_LL,
		LowLevelKeyboardProc,
		nullptr,
		0);
	if (!GlobalHook) {
		return -1;
	}

	while (GetMessage(&message, nullptr, 0, 0)) {
		if (message.message == WM_QUIT) {
			break;
		}
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
	UnhookWindowsHookEx(GlobalHook);
	return 0;
}

[[nodiscard]] GlobalShortcut MakeShortcut(
		std::vector<WinKeyDescriptor> descriptors) {
	return std::make_shared<GlobalShortcutValueWin>(std::move(descriptors));
}

[[nodiscard]] bool Matches(
		const std::vector<WinKeyDescriptor> &sorted,
		const base::flat_set<WinKeyDescriptor> &down) {
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

} // namespace

std::unique_ptr<GlobalShortcutManager> CreateGlobalShortcutManager() {
	return std::make_unique<GlobalShortcutManagerWin>();
}

GlobalShortcutValueWin::GlobalShortcutValueWin(
	std::vector<WinKeyDescriptor> descriptors)
: _descriptors(std::move(descriptors)) {
	Expects(!_descriptors.empty());
}

QString GlobalShortcutValueWin::toDisplayString() {
	auto result = QStringList();
	result.reserve(_descriptors.size());
	for (const auto descriptor : _descriptors) {
		result.push_back(KeyName(descriptor));
	}
	return result.join(" + ");
}

QByteArray GlobalShortcutValueWin::serialize() {
	static_assert(sizeof(WinKeyDescriptor) == 2 * sizeof(uint32));

	const auto size = sizeof(WinKeyDescriptor) * _descriptors.size();
	auto result = QByteArray(size, Qt::Uninitialized);
	memcpy(result.data(), _descriptors.data(), size);
	return result;
}

GlobalShortcutManagerWin::GlobalShortcutManagerWin() {
	std::unique_lock lock{ GlobalMutex };
	Managers.push_back(this);
	lock.unlock();

	if (!ThreadHandle) {
		ThreadEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		ThreadHandle = CreateThread(
			nullptr,
			0,
			&RunThread,
			nullptr,
			0,
			&ThreadId);
	}
}

GlobalShortcutManagerWin::~GlobalShortcutManagerWin() {
	std::unique_lock lock{ GlobalMutex };
	Managers.erase(ranges::remove(Managers, not_null{ this }), end(Managers));
	const auto stopThread = Managers.empty();
	lock.unlock();

	if (stopThread) {
		WaitForSingleObject(ThreadEvent, INFINITE);
		PostThreadMessage(ThreadId, WM_QUIT, 0, 0);
		WaitForSingleObject(ThreadHandle, INFINITE);
		CloseHandle(ThreadHandle);
		CloseHandle(ThreadEvent);
		ThreadHandle = ThreadEvent = nullptr;
		ThreadId = 0;
	}
}

void GlobalShortcutManagerWin::startRecording(
		Fn<void(GlobalShortcut)> progress,
		Fn<void(GlobalShortcut)> done) {
	Expects(done != nullptr);

	_recordingDown.clear();
	_recordingUp.clear();
	_recording = true;
	_recordingProgress = std::move(progress);
	_recordingDone = std::move(done);
}

void GlobalShortcutManagerWin::stopRecording() {
}

void GlobalShortcutManagerWin::startWatching(
		GlobalShortcut shortcut,
		Fn<void(bool pressed)> callback) {
	Expects(shortcut != nullptr);
	Expects(callback != nullptr);

	const auto i = ranges::find(_watchlist, shortcut, &Watch::shortcut);
	if (i != end(_watchlist)) {
		i->callback = std::move(callback);
	} else {
		auto sorted = static_cast<GlobalShortcutValueWin*>(
			shortcut.get())->descriptors();
		std::sort(begin(sorted), end(sorted));
		_watchlist.push_back(Watch{
			std::move(shortcut),
			std::move(sorted),
			std::move(callback)
		});
	}
}

void GlobalShortcutManagerWin::stopWatching(GlobalShortcut shortcut) {
	const auto i = ranges::find(_watchlist, shortcut, &Watch::shortcut);
	if (i != end(_watchlist)) {
		_watchlist.erase(i);
	}
	_pressed.erase(ranges::find(_pressed, shortcut), end(_pressed));
}

GlobalShortcut GlobalShortcutManagerWin::shortcutFromSerialized(
		QByteArray serialized) {
	const auto single = sizeof(WinKeyDescriptor);
	if (serialized.isEmpty() || serialized.size() % single) {
		return nullptr;
	}
	auto count = serialized.size() / single;
	auto list = std::vector<WinKeyDescriptor>(count);
	memcpy(list.data(), serialized.constData(), serialized.size());
	return MakeShortcut(std::move(list));
}

void GlobalShortcutManagerWin::schedule(
		WinKeyDescriptor descriptor,
		bool down) {
	InvokeQueued(this, [=] { process(descriptor, down); });
}

void GlobalShortcutManagerWin::process(
		WinKeyDescriptor descriptor,
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
			const auto win = static_cast<GlobalShortcutValueWin*>(i->get());
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

void GlobalShortcutManagerWin::processRecording(
		WinKeyDescriptor descriptor,
		bool down) {
	if (down) {
		processRecordingPress(descriptor);
	} else {
		processRecordingRelease(descriptor);
	}
}

void GlobalShortcutManagerWin::processRecordingPress(
		WinKeyDescriptor descriptor) {
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

void GlobalShortcutManagerWin::processRecordingRelease(
		WinKeyDescriptor descriptor) {
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

void GlobalShortcutManagerWin::finishRecording() {
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
