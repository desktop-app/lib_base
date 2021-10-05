// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/platform/win/base_windows_h.h"

#define _ROAPI_ // we don't want them from .dll
#include <roapi.h>
#undef _ROAPI_

#include <wrl/client.h>
#include <strsafe.h>
#include <intsafe.h>

namespace base::Platform {

void InitWRL();

inline HRESULT(__stdcall *RoGetActivationFactory)(
	_In_ HSTRING activatableClassId,
	_In_ REFIID iid,
	_COM_Outptr_ void ** factory);

inline HRESULT(__stdcall *RoActivateInstance)(
	_In_ HSTRING activatableClassId,
	_COM_Outptr_ IInspectable** instance);

inline HRESULT(__stdcall *RoRegisterActivationFactories)(
	_In_reads_(count) HSTRING* activatableClassIds,
	_In_reads_(count) PFNGETACTIVATIONFACTORY* activationFactoryCallbacks,
	_In_ UINT32 count,
	_Out_ RO_REGISTRATION_COOKIE* cookie);

inline void(__stdcall *RoRevokeActivationFactories)(
	_In_ RO_REGISTRATION_COOKIE cookie);

inline HRESULT(__stdcall *WindowsCreateString)(
	_In_reads_opt_(length) PCNZWCH sourceString,
	UINT32 length,
	_Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING* string);

inline HRESULT(__stdcall *WindowsCreateStringReference)(
	_In_reads_opt_(length + 1) PCWSTR sourceString,
	UINT32 length,
	_Out_ HSTRING_HEADER * hstringHeader,
	_Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING *string);

inline HRESULT(__stdcall *WindowsDeleteString)(
	_In_opt_ HSTRING string);

inline PCWSTR(__stdcall *WindowsGetStringRawBuffer)(
	_In_opt_ HSTRING string,
	_Out_opt_ UINT32* length);

inline BOOL(__stdcall *WindowsIsStringEmpty)(
	_In_opt_ HSTRING string);

inline HRESULT(__stdcall *WindowsStringHasEmbeddedNull)(
	_In_opt_ HSTRING string,
	_Out_ BOOL* hasEmbedNull);

inline BOOL(__stdcall *RoOriginateErrorW)(
	_In_ HRESULT error,
	_In_ UINT cchMax,
	_When_(cchMax == 0, _In_reads_or_z_opt_(MAX_ERROR_MESSAGE_CHARS))
	_When_(cchMax > 0 && cchMax < MAX_ERROR_MESSAGE_CHARS, _In_reads_or_z_(cchMax))
	_When_(cchMax >= MAX_ERROR_MESSAGE_CHARS, _In_reads_or_z_(MAX_ERROR_MESSAGE_CHARS)) PCWSTR message);

inline BOOL(__stdcall *RoOriginateError)(
	_In_ HRESULT error,
	_In_opt_ HSTRING message);

[[nodiscard]] inline bool SupportsWRL() {
	InitWRL();
	return (RoGetActivationFactory != nullptr)
		&& (WindowsCreateString != nullptr)
		&& (WindowsCreateStringReference != nullptr)
		&& (WindowsDeleteString != nullptr);
}

namespace details {

template <class T>
_Check_return_ __inline HRESULT GetActivationFactory(
		_In_ HSTRING activatableClassId,
		_COM_Outptr_ T** factory) {
	return RoGetActivationFactory
		? RoGetActivationFactory(activatableClassId, IID_INS_ARGS(factory))
		: E_FAIL;
}

template <class T>
_Check_return_ __inline  HRESULT ActivateInstance(
		_In_ HSTRING activatableClassId,
		_COM_Outptr_ T** instance) {
	*instance = nullptr;

	auto pInspectable = (IInspectable*)nullptr;
	auto hr = RoActivateInstance
		? RoActivateInstance(activatableClassId, &pInspectable)
		: E_FAIL;
	if (SUCCEEDED(hr)) {
		if (__uuidof(T) == __uuidof(IInspectable)) {
			*instance = static_cast<T*>(pInspectable);
		} else {
			hr = pInspectable->QueryInterface(IID_INS_ARGS(instance));
			pInspectable->Release();
		}
	}
	return hr;
}

} // namespace details

template <typename T>
inline HRESULT GetActivationFactory(
		_In_ HSTRING activatableClassId,
		_Inout_ Microsoft::WRL::Details::ComPtrRef<T> factory) throw() {
	return details::GetActivationFactory(
		activatableClassId,
		factory.ReleaseAndGetAddressOf());
}

template <typename T>
inline HRESULT ActivateInstance(
		_In_ HSTRING activatableClassId,
		_Inout_::Microsoft::WRL::Details::ComPtrRef<T> instance) throw() {
	return details::ActivateInstance(
		activatableClassId,
		instance.ReleaseAndGetAddressOf());
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
