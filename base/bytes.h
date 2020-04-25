// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/basic_types.h"
#include <gsl/gsl>
#include <gsl/gsl_byte>
#include <vector>
#include <array>

namespace bytes {

using type = gsl::byte;
using span = gsl::span<type>;
using const_span = gsl::span<const type>;
using vector = std::vector<type>;

template <gsl::index Size>
using array = std::array<type, Size>;

inline span make_detached_span(QByteArray &container) {
	return gsl::as_writable_bytes(gsl::make_span(container));
}

template <
	typename Container,
	typename = std::enable_if_t<
		!std::is_const_v<Container>
		&& !std::is_same_v<Container, QByteArray>>>
inline span make_span(Container &container) {
	return gsl::as_writable_bytes(gsl::make_span(container));
}

template<>
inline span make_span<span, void>(span &container) {
	return container;
}

template <typename Container>
inline const_span make_span(const Container &container) {
	return gsl::as_bytes(gsl::make_span(container));
}

inline const_span make_span(const_span &container) {
	return container;
}

template <typename Type, std::ptrdiff_t Extent>
inline span make_span(gsl::span<Type, Extent> container) {
	return gsl::as_writable_bytes(container);
}

template <typename Type, std::ptrdiff_t Extent>
inline const_span make_span(gsl::span<const Type, Extent> container) {
	return gsl::as_bytes(container);
}

template <typename Type>
inline span make_span(Type *value, std::size_t count) {
	return gsl::as_writable_bytes(gsl::make_span(value, count));
}

template <typename Type>
inline const_span make_span(const Type *value, std::size_t count) {
	return gsl::as_bytes(gsl::make_span(value, count));
}

template <typename Type>
inline span object_as_span(Type *value) {
	return bytes::make_span(value, 1);
}

template <typename Type>
inline const_span object_as_span(const Type *value) {
	return bytes::make_span(value, 1);
}

template <typename Container>
inline vector make_vector(const Container &container) {
	const auto buffer = bytes::make_span(container);
	return { buffer.begin(), buffer.end() };
}

inline void copy(span destination, const_span source) {
	Expects(destination.size() >= source.size());

	memcpy(destination.data(), source.data(), source.size());
}

inline void move(span destination, const_span source) {
	Expects(destination.size() >= source.size());

	memmove(destination.data(), source.data(), source.size());
}

inline void set_with_const(span destination, type value) {
	memset(
		destination.data(),
		gsl::to_integer<unsigned char>(value),
		destination.size());
}

inline int compare(const_span a, const_span b) {
	const auto aSize = a.size(), bSize = b.size();
	return (aSize > bSize)
		? 1
		: (aSize < bSize)
		? -1
		: memcmp(a.data(), b.data(), aSize);
}

namespace details {

template <typename Arg>
std::size_t spansLength(Arg &&arg) {
	return bytes::make_span(arg).size();
}

template <typename Arg, typename ...Args>
std::size_t spansLength(Arg &&arg, Args &&...args) {
	return bytes::make_span(arg).size() + spansLength(args...);
}

template <typename Arg>
void spansAppend(span destination, Arg &&arg) {
	bytes::copy(destination, bytes::make_span(arg));
}

template <typename Arg, typename ...Args>
void spansAppend(span destination, Arg &&arg, Args &&...args) {
	const auto data = bytes::make_span(arg);
	bytes::copy(destination, data);
	spansAppend(destination.subspan(data.size()), args...);
}

} // namespace details

template <
	typename ...Args,
	typename = std::enable_if_t<(sizeof...(Args) > 1)>>
vector concatenate(Args &&...args) {
	const auto size = details::spansLength(args...);
	auto result = vector(size);
	details::spansAppend(make_span(result), args...);
	return result;
}

template <
	typename SpanRange>
vector concatenate(SpanRange args) {
	auto size = std::size_t(0);
	for (const auto &arg : args) {
		size += bytes::make_span(arg).size();
	}
	auto result = vector(size);
	auto buffer = make_span(result);
	for (const auto &arg : args) {
		const auto part = bytes::make_span(arg);
		bytes::copy(buffer, part);
		buffer = buffer.subspan(part.size());
	}
	return result;
}

// Implemented in base/openssl_help.h
void set_random(span destination);

} // namespace bytes
