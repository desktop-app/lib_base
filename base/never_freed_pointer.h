// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/assertion.h"

namespace base {

// This pointer is used for global non-POD variables that are allocated
// on demand by createIfNull(lambda) and are never automatically freed.
template <typename T>
class NeverFreedPointer {
public:
	NeverFreedPointer() = default;
	NeverFreedPointer(const NeverFreedPointer<T> &other) = delete;
	NeverFreedPointer &operator=(const NeverFreedPointer<T> &other) = delete;

	template <typename... Args>
	void createIfNull(Args&&... args) {
		if (isNull()) {
			reset(new T(std::forward<Args>(args)...));
		}
	};

	T *data() const {
		return _p;
	}
	T *release() {
		return base::take(_p);
	}
	void reset(T *p = nullptr) {
		delete _p;
		_p = p;
	}
	bool isNull() const {
		return data() == nullptr;
	}

	void clear() {
		reset();
	}
	T *operator->() const {
		return data();
	}
	T &operator*() const {
		Assert(!isNull());
		return *data();
	}
	explicit operator bool() const {
		return !isNull();
	}

private:
	T *_p;

};

} // namespace base
