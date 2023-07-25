// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/qt/qt_tab_key.h"

#include "base/event_filter.h"

#include <QApplication>
#include <QKeyEvent>
#include <QWidget>

namespace base {

bool FocusNextPrevChildBlocked(not_null<QWidget*> widget, bool next) {
	const auto skip = [&](QWidget *w) {
		return next ? w->nextInFocusChain() : w->previousInFocusChain();
	};
	const auto focused = QApplication::focusWidget();
	const auto start = focused ? focused : widget.get();
	auto w = skip(start);
	while (w && w != start) {
		if ((w->focusPolicy() & Qt::TabFocus) && widget->isAncestorOf(w)) {
			break;
		}
		w = skip(w);
	}
	if (w && w != start) {
		w->setFocus(next ? Qt::TabFocusReason : Qt::BacktabFocusReason);
	}
	return true;
}

void DisableTabKey(QWidget *widget) {
	if (!widget) {
		return;
	}
	install_event_filter(widget, [=](not_null<QEvent*> e) {
		if (e->type() == QEvent::KeyPress) {
			const auto key = static_cast<QKeyEvent*>(e.get())->key();
			if ((key == Qt::Key_Tab) || (key == Qt::Key_Backtab)) {
				return base::EventFilterResult::Cancel;
			}
		}
		return base::EventFilterResult::Continue;
	});
}

} // namespace base
