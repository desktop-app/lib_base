// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include <compare>
#include <gsl/pointers>

#include <QString>

template <typename P>
[[nodiscard]] inline std::strong_ordering operator<=>(
		const gsl::not_null<P> &a,
		const gsl::not_null<P> &b) noexcept {
	return a.get() <=> b.get();
}

[[nodiscard]] inline std::strong_ordering operator<=>(
		const QString &a,
		const QString &b) noexcept {
	return a.compare(b) <=> 0;
}
