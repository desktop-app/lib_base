// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include <QtCore/QEvent>
#include <QtCore/QCoreApplication>

#include "base/basic_types.h"

namespace base {

class InvokeQueuedEvent : public QEvent {
public:
	static auto Type() {
		static const auto Result = QEvent::Type(QEvent::registerEventType());
		return Result;
	}

	explicit InvokeQueuedEvent(FnMut<void()> &&method)
	: QEvent(Type())
	, _method(std::move(method)) {
	}

	void invoke() {
		_method();
	}

private:
	FnMut<void()> _method;

};

} // namespace base

template <typename Lambda>
inline void InvokeQueued(const QObject *context, Lambda &&lambda) {
	QCoreApplication::postEvent(
		const_cast<QObject*>(context),
		new base::InvokeQueuedEvent(std::forward<Lambda>(lambda)));
}

class SingleQueuedInvokation : public QObject {
public:
	SingleQueuedInvokation(Fn<void()> callback) : _callback(callback) {
	}
	void call() {
		if (_pending.testAndSetAcquire(0, 1)) {
			InvokeQueued(this, [this] {
				if (_pending.testAndSetRelease(1, 0)) {
					_callback();
				}
			});
		}
	}

private:
	Fn<void()> _callback;
	QAtomicInt _pending = { 0 };

};
