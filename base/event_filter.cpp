// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/event_filter.h"

#include <QtCore/QPointer>

namespace base {
namespace details {

EventFilter::EventFilter(
	not_null<QObject*> parent,
	not_null<QObject*> object,
	Fn<EventFilterResult(not_null<QEvent*>)> filter)
: QObject(parent)
, _filter(std::move(filter)) {
	object->installEventFilter(this);
}

bool EventFilter::eventFilter(QObject *watched, QEvent *event) {
	return (_filter(event) == EventFilterResult::Cancel);
}

} // namespace details

not_null<QObject*> install_event_filter(
		not_null<QObject*> object,
		Fn<EventFilterResult(not_null<QEvent*>)> filter) {
	return install_event_filter(object, object, std::move(filter));
}

not_null<QObject*> install_event_filter(
		not_null<QObject*> context,
		not_null<QObject*> object,
		Fn<EventFilterResult(not_null<QEvent*>)> filter) {
	return new details::EventFilter(context, object, std::move(filter));
}

void install_event_filter(
		not_null<QObject*> object,
		Fn<EventFilterResult(not_null<QEvent*>)> filter,
		rpl::lifetime &lifetime) {
	// Not safe in case object is deleted before lifetime.
	//
	//lifetime.make_state<details::EventFilter>(
	//	object,
	//	object,
	//	std::move(filter));

	const auto raw = install_event_filter(object, std::move(filter));
	lifetime.add([weak = QPointer<QObject>(raw.get())] {
		if (const auto strong = weak.data()) {
			delete strong;
		}
	});
}

} // namespace Core
