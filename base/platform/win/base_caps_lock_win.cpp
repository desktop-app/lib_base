// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/win/base_caps_lock_win.h"

#include "base/platform/win/base_windows_h.h"
#include "base/global_shortcuts.h"
#include "base/integration.h"

#include <QtCore/QAbstractNativeEventFilter>

namespace base::Platform {
namespace {

template <typename Callback>
class Filter final : public QAbstractNativeEventFilter {
public:
	Filter(Callback &&callback) : _callback(std::move(callback)) {
	}

	bool nativeEventFilter(
			const QByteArray &eventType,
			void *message,
			long *result) override {
		const auto msg = static_cast<MSG*>(message);
		if (msg->message == WM_KEYDOWN && msg->wParam == VK_CAPITAL) {
			base::Integration::Instance().enterFromEventLoop(_callback);
		}
		return false;
	}

private:
	Callback _callback;

};

} // namespace

[[nodiscard]] bool CapsLockStateKnown() {
	return true;
}

[[nodiscard]] bool CapsLockOn() {
	return (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
}

[[nodiscard]] rpl::producer<bool> CapsLockOnValue() {
	return [=](auto consumer) {
		auto lifetime = rpl::lifetime();

		struct State {
			std::unique_ptr<base::GlobalShortcutManager> manager;
			bool enabled = false;
			bool pressed = false;
		};

		const auto state = lifetime.make_state<State>(State{
			.manager = base::CreateGlobalShortcutManager(),
			.enabled = CapsLockOn(),
		});
		const auto manager = state->manager.get();
		manager->startWatchingAll([=](const base::GlobalShortcut &down) {
			const auto pressed = ranges::contains(
				down->nativeKeys(),
				VK_CAPITAL,
				[](uint64 v) { return int(v >> 32); });
			if (state->pressed != pressed) {
				state->pressed = pressed;
				if (pressed) {
					const auto enabled = CapsLockOn();
					if (state->enabled != enabled) {
						state->enabled = enabled;
						consumer.put_next_copy(enabled);
					}
				}
			}
		});

		consumer.put_next_copy(state->enabled);
		return lifetime;
	};
}

} // namespace base::Platform
