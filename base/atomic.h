// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include <mutex>
#include <memory>
#include <condition_variable>

namespace base {

// Helper for macOS < 11.0 and GCC < 11 that don't have wait/notify.

#if defined __cpp_lib_atomic_wait && !defined Q_OS_MAC

using std::atomic;
using std::atomic_notify_all;

#else // __cpp_lib_atomic_wait

template <typename Type>
class atomic final {
public:
	atomic(Type value) : _value(value) {
	}
	atomic &operator=(Type value) {
		_value = value;
		return *this;
	}

	[[nodiscard]] Type load() const {
		return _value.load();
	}

	void wait(Type whileEquals) {
		if (_value == whileEquals) {
			std::unique_lock<std::mutex> lock(_mutex);
			while (_value == whileEquals) {
				_condition.wait(lock);
			}
		}
	}

	friend inline bool operator==(const atomic &a, Type b) {
		return (a._value == b);
	}

	friend void atomic_notify_all(atomic *value) {
		std::unique_lock<std::mutex> lock(value->_mutex);
		value->_condition.notify_all();
	}

private:
	std::atomic<Type> _value;
	std::mutex _mutex;
	std::condition_variable _condition;

};

#endif // __cpp_lib_atomic_wait

} // namespace base
