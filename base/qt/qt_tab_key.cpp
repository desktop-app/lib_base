// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/qt/qt_tab_key.h"

#include "base/event_filter.h"

#include <QEvent>
#include <QKeyEvent>
#include <QWidget>

namespace base {

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
