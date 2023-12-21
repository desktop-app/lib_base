// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/algorithm.h"

#include <QtCore/QPointer>
#include <QtCore/QObject>

namespace base {

template <typename T>
class unique_qptr {
public:
	unique_qptr() = default;
	unique_qptr(std::nullptr_t) noexcept {
	}
	explicit unique_qptr(T *pointer) noexcept
	: _object(pointer) {
	}

	unique_qptr(const unique_qptr &other) = delete;
	unique_qptr &operator=(const unique_qptr &other) = delete;
	unique_qptr(unique_qptr &&other) noexcept
	: _object(base::take(other._object)) {
	}
	unique_qptr &operator=(unique_qptr &&other) noexcept {
		if (_object != other._object) {
			destroy();
			_object = base::take(other._object);
		}
		return *this;
	}

	template <
		typename U,
		typename = std::enable_if_t<std::is_base_of_v<T, U>>>
	unique_qptr(unique_qptr<U> &&other) noexcept
	: _object(base::take(other._object)) {
	}

	template <
		typename U,
		typename = std::enable_if_t<std::is_base_of_v<T, U>>>
	unique_qptr &operator=(unique_qptr<U> &&other) noexcept {
		if (_object != other._object) {
			destroy();
			_object = base::take(other._object);
		}
		return *this;
	}

	unique_qptr &operator=(std::nullptr_t) noexcept {
		destroy();
		return *this;
	}

	template <typename ...Args>
	explicit unique_qptr(std::in_place_t, Args &&...args)
	: _object(new T(std::forward<Args>(args)...)) {
	}

	template <typename ...Args>
	T *emplace(Args &&...args) {
		reset(new T(std::forward<Args>(args)...));
		return get();
	}

	void reset(T *value = nullptr) noexcept {
		if (_object != value) {
			destroy();
			_object = value;
		}
	}

	T *get() const noexcept {
		return static_cast<T*>(_object.data());
	}
	operator T*() const noexcept {
		return get();
	}

	T *release() noexcept {
		return static_cast<T*>(base::take(_object).data());
	}

	explicit operator bool() const noexcept {
		return _object != nullptr;
	}

	T *operator->() const noexcept {
		return get();
	}
	T &operator*() const noexcept {
		return *get();
	}

	~unique_qptr() noexcept {
		destroy();
	}

private:
	void destroy() noexcept {
		delete base::take(_object).data();
	}

	template <typename U>
	friend class unique_qptr;

	QPointer<QObject> _object;

};

template <typename T, typename ...Args>
inline unique_qptr<T> make_unique_q(Args &&...args) {
	return unique_qptr<T>(std::in_place, std::forward<Args>(args)...);
}

} // namespace base
