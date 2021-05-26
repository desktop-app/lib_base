// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/build_config.h"
#include "base/ordered_set.h"
#include "base/unique_function.h"
#include "base/functors.h"
#include "base/required.h"

#include <QtGlobal>
#include <QtCore/QByteArray>
#include <QtCore/QString>

#include <string>
#include <exception>
#include <memory>
#include <ctime>
#include <functional>
#include <gsl/gsl>

namespace func = base::functors;

using gsl::not_null;
using index_type = gsl::index;
using size_type = gsl::index;
using base::required;

template <typename Signature>
using Fn = std::function<Signature>;

template <typename Signature>
using FnMut = base::unique_function<Signature>;

//using uchar = unsigned char; // Qt has uchar
using int8 = qint8;
using uint8 = quint8;
using int16 = qint16;
using uint16 = quint16;
using int32 = qint32;
using uint32 = quint32;
using int64 = qint64;
using uint64 = quint64;
using float32 = float;
using float64 = double;

using TimeId = int32;

#ifdef _MSC_VER
#define DESKTOP_APP_USE_NO_ALLOCATION_LITERAL
#endif // _MSC_VER

#ifdef DESKTOP_APP_USE_NO_ALLOCATION_LITERAL

namespace base::details {

struct LiteralResolver {
	template <size_t N>
	constexpr LiteralResolver(const char16_t (&text)[N])
	: utf16text(text)
	, length(N) {
	}
	template <size_t N>
	constexpr LiteralResolver(const char (&text)[N])
	: utf8text(text)
	, length(N)
	, utf8(true) {
	}
	constexpr auto operator<=>(const LiteralResolver &) const = default;

	const char16_t *utf16text = nullptr;
	const char *utf8text = nullptr;
	size_t length = 0;
	bool utf8 = false;
};

template <size_t N>
struct StaticStringData {
	template <std::size_t... I>
	constexpr StaticStringData(
		const char16_t *text,
		std::index_sequence<I...>)
	: data Q_STATIC_STRING_DATA_HEADER_INITIALIZER(N)
	, text{ text[I]... } {
	}
    QArrayData data;
	char16_t text[N + 1];

    QStringData *pointer() {
        Q_ASSERT(data.ref.isStatic());
        return static_cast<QStringData*>(&data);
    }
};

template <size_t N>
struct StaticByteArrayData {
	template <std::size_t... I>
	constexpr StaticByteArrayData(
		const char *text,
		std::index_sequence<I...>)
	: data Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER(N)
	, text{ text[I]... } {
	}
	QByteArrayData data;
	char text[N + 1];

    QByteArrayData *pointer() {
        Q_ASSERT(data.ref.isStatic());
        return &data;
    }
};

template <LiteralResolver Resolve>
using q_literal_type = std::conditional_t<Resolve.utf8, QByteArray, QString>;

} // namespace base::details

template <base::details::LiteralResolver Resolve>
base::details::q_literal_type<Resolve> operator""_q() {
	using namespace base::details;
	if constexpr (Resolve.utf8) {
		static auto Literal = StaticByteArrayData<Resolve.length>(
			Resolve.utf8text,
			std::make_index_sequence<Resolve.length + 1>{});
		return QByteArray{ QByteArrayDataPtr{ Literal.pointer() } };
	} else {
		static auto Literal = StaticStringData<Resolve.length>(
			Resolve.utf16text,
			std::make_index_sequence<Resolve.length + 1>{});
		return QString{ QStringDataPtr{ Literal.pointer() } };
	}
};

#else // DESKTOP_APP_USE_NO_ALLOCATION_LITERAL

[[nodiscard]] inline QByteArray operator""_q(
		const char *data,
		std::size_t size) {
	return QByteArray::fromRawData(data, size);
}

[[nodiscard]] inline QString operator""_q(
		const char16_t *data,
		std::size_t size) {
	return QString::fromRawData(
		reinterpret_cast<const QChar*>(data),
		size);
}

#endif // DESKTOP_APP_USE_NO_ALLOCATION_LITERAL
