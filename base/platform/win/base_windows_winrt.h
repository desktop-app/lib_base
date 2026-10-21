// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/debug_log.h"

#include <source_location>

// winrt::guid defines a constructor from GUID only if knows IUnknown.
#include <Unknwnbase.h>

// class has virtual functions, but destructor is not virtual
#pragma warning(push)
#pragma warning(disable:4265)
#include <winrt/base.h>
#pragma warning(pop)

namespace base::WinRT {
namespace details {

template <typename Method>
inline constexpr bool ReturnsVoid = std::is_same_v<
	std::invoke_result_t<Method>,
	void>;

template <typename Method>
using TryResult = std::conditional_t<
	ReturnsVoid<Method>,
	bool,
	std::optional<std::invoke_result_t<Method>>>;

void LogError(
	HRESULT code,
	const QString &message,
	const std::source_location &location);

} // namespace details

[[nodiscard]] bool Supported();

template <typename Method>
inline details::TryResult<Method> TryNoCheck(
		Method &&method,
		HRESULT *errorCode = nullptr,
		const std::source_location &location
			= std::source_location::current()) noexcept {
	if (errorCode) {
		*errorCode = S_OK;
	}
	const auto fail = [&](HRESULT code, const QString &message) {
		if (errorCode) {
			*errorCode = code;
		}
		details::LogError(code, message, location);
		return details::TryResult<Method>();
	};
	try {
		if constexpr (details::ReturnsVoid<Method>) {
			method();
			return true;
		} else {
			return method();
		}
	} catch (const std::bad_alloc &) {
		// C++/WinRT maps *any* E_OUTOFMEMORY HRESULT to std::bad_alloc,
		// including one coming back from an out-of-process COM server or
		// from the RPC layer. That is not our own allocation failure, so
		// report "no result" like every other failed WinRT call.
		return fail(E_OUTOFMEMORY, "Could not allocate (E_OUTOFMEMORY).");
	} catch (const winrt::hresult_error &error) {
		return fail(
			error.code(),
			QString::fromWCharArray(error.message().c_str()));
	} catch (...) {
		return fail(E_FAIL, "Unknown exception.");
	}
}

template <typename Method>
inline details::TryResult<Method> Try(
		Method &&method,
		const std::source_location &location
			= std::source_location::current()) noexcept {
	if (!Supported()) {
		return {};
	}
	return TryNoCheck(std::forward<Method>(method), nullptr, location);
}

template <typename Interface>
auto TryCreateInstance(
		const winrt::guid &clsid,
		uint32_t context = 0x1 /*CLSCTX_INPROC_SERVER*/,
		void *outer = nullptr,
		HRESULT *errorCode = nullptr,
		const std::source_location &location = std::source_location::current()) {
	return TryNoCheck([&] {
		return winrt::create_instance<Interface>(clsid, context, outer);
	}, errorCode, location).value_or(winrt::com_ptr<Interface>());
}

} // namespace base::WinRT
