// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

namespace base {

template <typename T>
struct required {
	constexpr required(T &&value) noexcept : _value(std::move(value)) {
	}
	constexpr required(const T &value) noexcept : _value(value) {
	}
	constexpr required &operator=(T &&value) noexcept {
		_value = std::move(value);
		return *this;
	}
	constexpr required &operator=(const T &value) noexcept {
		_value = value;
		return *this;
	}
	constexpr required(required &&) = default;
	constexpr required(const required &) = default;
	constexpr required &operator=(required &&) = default;
	constexpr required &operator=(const required &) = default;

	[[nodiscard]] constexpr T &operator*() noexcept {
		return _value;
	}
	[[nodiscard]] constexpr const T &operator*() const noexcept {
		return _value;
	}
	[[nodiscard]] constexpr T &value() noexcept {
		return _value;
	}
	[[nodiscard]] constexpr const T &value() const noexcept {
		return _value;
	}
	[[nodiscard]] constexpr T *operator->() noexcept {
		return &_value;
	}
	[[nodiscard]] constexpr const T *operator->() const noexcept {
		return &_value;
	}
	[[nodiscard]] constexpr T *get() noexcept {
		return &_value;
	}
	[[nodiscard]] constexpr const T *get() const noexcept {
		return &_value;
	}
	[[nodiscard]] operator T&() noexcept {
		return _value;
	}
	[[nodiscard]] operator const T&() const noexcept {
		return _value;
	}

private:
	T _value;

};

} // namespace base
