// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#ifdef _DEBUG

#include "base/qt/qt_debug_hotkey.h"

#include "base/event_filter.h"

#include <QGuiApplication>
#include <QKeyEvent>
#include <QWindow>

namespace base {

void DebugHotkeyPresses(Fn<void()> callback) {
	const auto window = QGuiApplication::allWindows().front();
	Assert(window != nullptr);

	base::install_event_filter(window, [=](not_null<QEvent*> e) {
		if (e->type() != QEvent::KeyPress) {
			return base::EventFilterResult::Continue;
		}
		const auto key = static_cast<QKeyEvent*>(e.get())->key();
		if (key != Qt::Key_F12) {
			return base::EventFilterResult::Continue;
		}
		callback();
		return base::EventFilterResult::Cancel;
	});
}

rpl::producer<> DebugHotkeyPresses() {
	const auto presses = std::make_shared<rpl::event_stream<>>();
	DebugHotkeyPresses([=] { presses->fire({}); });
	return presses->events();
}

} // namespace base

#endif // !_DEBUG
