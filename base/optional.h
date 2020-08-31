// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include <optional>
#include <gsl/gsl_assert>
#include "base/variant.h"

namespace base {

template <typename... Types>
class optional_variant {
public:
	optional_variant() : _impl(v::null) {
	}
	optional_variant(const optional_variant &other) : _impl(other._impl) {
	}
	optional_variant(optional_variant &&other) : _impl(std::move(other._impl)) {
	}
	template <typename T, typename = std::enable_if_t<!std::is_base_of<optional_variant, std::decay_t<T>>::value>>
	optional_variant(T &&value) : _impl(std::forward<T>(value)) {
	}
	optional_variant &operator=(const optional_variant &other) {
		_impl = other._impl;
		return *this;
	}
	optional_variant &operator=(optional_variant &&other) {
		_impl = std::move(other._impl);
		return *this;
	}
	template <typename T, typename = std::enable_if_t<!std::is_base_of<optional_variant, std::decay_t<T>>::value>>
	optional_variant &operator=(T &&value) {
		_impl = std::forward<T>(value);
		return *this;
	}

	bool has_value() const {
		return !is<v::null_t>();
	}
	explicit operator bool() const {
		return has_value();
	}
	bool operator==(const optional_variant &other) const {
		return _impl == other._impl;
	}
	bool operator!=(const optional_variant &other) const {
		return _impl != other._impl;
	}
	bool operator<(const optional_variant &other) const {
		return _impl < other._impl;
	}
	bool operator<=(const optional_variant &other) const {
		return _impl <= other._impl;
	}
	bool operator>(const optional_variant &other) const {
		return _impl > other._impl;
	}
	bool operator>=(const optional_variant &other) const {
		return _impl >= other._impl;
	}

	template <typename T, typename... Args>
	T &set(Args &&...args) {
		_impl.template set<T>(std::forward<Args>(args)...);
		return get_unchecked<T>();
	}
	void clear() {
		_impl.template set<v::null_t>();
	}

	template <typename T>
	decltype(auto) is() const {
		return v::is<T>(_impl);
	}
	template <typename T>
	decltype(auto) get_unchecked() {
		return std::get<T>(_impl);
	}
	template <typename T>
	decltype(auto) get_unchecked() const {
		return std::get<T>(_impl);
	}

	template <typename ...Methods>
	decltype(auto) match(Methods &&...methods) {
		return v::match(_impl, std::forward<Methods>(methods)...);
	}
	template <typename ...Methods>
	decltype(auto) match(Methods &&...methods) const {
		return v::match(_impl, std::forward<Methods>(methods)...);
	}

private:
	std::variant<v::null_t, Types...> _impl;

};

template <typename T, typename... Types>
inline T *get_if(optional_variant<Types...> *v) {
	return (v && v->template is<T>()) ? &v->template get_unchecked<T>() : nullptr;
}

template <typename T, typename... Types>
inline const T *get_if(const optional_variant<Types...> *v) {
	return (v && v->template is<T>()) ? &v->template get_unchecked<T>() : nullptr;
}

template <typename ...Types, typename ...Methods>
inline decltype(auto) match(
		optional_variant<Types...> &value,
		Methods &&...methods) {
	return value.match(std::forward<Methods>(methods)...);
}

template <typename ...Types, typename ...Methods>
inline decltype(auto) match(
		const optional_variant<Types...> &value,
		Methods &&...methods) {
	return value.match(std::forward<Methods>(methods)...);
}

template <typename Type>
struct optional_wrap_once {
	using type = std::optional<Type>;
};

template <typename Type>
struct optional_wrap_once<std::optional<Type>> {
	using type = std::optional<Type>;
};

template <typename Type>
using optional_wrap_once_t = typename optional_wrap_once<std::decay_t<Type>>::type;

template <typename Type>
struct optional_chain_result {
	using type = optional_wrap_once_t<Type>;
};

template <>
struct optional_chain_result<void> {
	using type = bool;
};

template <typename Type>
using optional_chain_result_t = typename optional_chain_result<Type>::type;

template <typename Type>
optional_wrap_once_t<Type> make_optional(Type &&value) {
	return optional_wrap_once_t<Type> { std::forward<Type>(value) };
}

} // namespace base

template <typename Type, typename Method>
inline auto operator|(const std::optional<Type> &value, Method method)
-> base::optional_chain_result_t<decltype(method(*value))> {
	if constexpr (std::is_same_v<decltype(method(*value)), void>) {
		return value ? (method(*value), true) : false;
	} else {
		return value
			? base::optional_chain_result_t<decltype(method(*value))>(
				method(*value))
			: std::nullopt;
	}
}
