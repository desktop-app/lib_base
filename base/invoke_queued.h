// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include <QtCore/QObject>

#include "base/basic_types.h"

template <typename Lambda>
inline void InvokeQueued(const QObject *context, Lambda &&lambda) {
	QMetaObject::invokeMethod(
		const_cast<QObject*>(context),
		std::forward<Lambda>(lambda),
		Qt::QueuedConnection);
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
