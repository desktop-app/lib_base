// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/platform/win/base_windows_h.h"

#include <roapi.h>
#include <wrl/client.h>
#include <strsafe.h>
#include <intsafe.h>

namespace base::Platform {

void InitWRL();

inline HRESULT(FAR STDAPICALLTYPE *RoGetActivationFactory)(
	_In_ HSTRING activatableClassId,
	_In_ REFIID iid,
	_COM_Outptr_ void ** factory);

inline HRESULT(FAR STDAPICALLTYPE *WindowsCreateStringReference)(
	_In_reads_opt_(length + 1) PCWSTR sourceString,
	UINT32 length,
	_Out_ HSTRING_HEADER * hstringHeader,
	_Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING *string);

inline HRESULT(FAR STDAPICALLTYPE *WindowsDeleteString)(
	_In_opt_ HSTRING string);

[[nodiscard]] inline bool SupportsWRL() {
	InitWRL();
	return (RoGetActivationFactory != nullptr)
		&& (WindowsCreateStringReference != nullptr)
		&& (WindowsDeleteString != nullptr);
}

namespace details {

template <class T>
_Check_return_ __inline HRESULT GetActivationFactory(_In_ HSTRING activatableClassId, _COM_Outptr_ T** factory) {
	return RoGetActivationFactory
		? RoGetActivationFactory(activatableClassId, IID_INS_ARGS(factory))
		: E_FAIL;
}

} // namespace details

template <typename T>
inline HRESULT GetActivationFactory(_In_ HSTRING activatableClassId, _Inout_ Microsoft::WRL::Details::ComPtrRef<T> factory) throw() {
	return details::GetActivationFactory(activatableClassId, factory.ReleaseAndGetAddressOf());
}

class StringReferenceWrapper {
public:
	StringReferenceWrapper(_In_reads_(length) PCWSTR stringRef, _In_ UINT32 length) throw() {
		HRESULT hr = WindowsCreateStringReference(stringRef, length, &_header, &_hstring);
		if (!SUCCEEDED(hr)) {
			RaiseException(static_cast<DWORD>(STATUS_INVALID_PARAMETER), EXCEPTION_NONCONTINUABLE, 0, nullptr);
		}
	}

	~StringReferenceWrapper() {
		WindowsDeleteString(_hstring);
	}

	template <size_t N>
	StringReferenceWrapper(_In_reads_(N) wchar_t const (&stringRef)[N]) throw() {
		UINT32 length = N - 1;
		HRESULT hr = WindowsCreateStringReference(stringRef, length, &_header, &_hstring);
		if (!SUCCEEDED(hr)) {
			RaiseException(static_cast<DWORD>(STATUS_INVALID_PARAMETER), EXCEPTION_NONCONTINUABLE, 0, nullptr);
		}
	}

	template <size_t _>
	StringReferenceWrapper(_In_reads_(_) wchar_t(&stringRef)[_]) throw() {
		UINT32 length;
		HRESULT hr = SizeTToUInt32(wcslen(stringRef), &length);
		if (!SUCCEEDED(hr)) {
			RaiseException(static_cast<DWORD>(STATUS_INVALID_PARAMETER), EXCEPTION_NONCONTINUABLE, 0, nullptr);
		}

		WindowsCreateStringReference(stringRef, length, &_header, &_hstring);
	}

	HSTRING Get() const throw() {
		return _hstring;
	}

private:
	HSTRING _hstring;
	HSTRING_HEADER _header;

};

} // namespace base::Platform
