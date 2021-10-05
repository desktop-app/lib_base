// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/win/base_windows_wrl.h"

#include "base/platform/win/base_windows_safe_library.h"
#include "base/platform/base_platform_info.h"

#define LOAD_SYMBOL(lib, name) ::base::Platform::LoadMethod(lib, #name, name)

namespace base::Platform {
namespace {

using namespace ::Platform;

} // namespace

void InitWRL() {
	static bool Inited = false;
	if (Inited) {
		return;
	}
	Inited = true;

	if (!IsWindows8OrGreater()) {
		return;
	}
	const auto combase = SafeLoadLibrary(L"combase.dll");
	LOAD_SYMBOL(combase, RoGetActivationFactory);
	LOAD_SYMBOL(combase, RoActivateInstance);
	LOAD_SYMBOL(combase, RoRegisterActivationFactories);
	LOAD_SYMBOL(combase, RoRevokeActivationFactories);
	LOAD_SYMBOL(combase, WindowsCreateString);
	LOAD_SYMBOL(combase, WindowsCreateStringReference);
	LOAD_SYMBOL(combase, WindowsDeleteString);
	LOAD_SYMBOL(combase, WindowsGetStringRawBuffer);
	LOAD_SYMBOL(combase, WindowsIsStringEmpty);
	LOAD_SYMBOL(combase, WindowsStringHasEmbeddedNull);
}

} // namespace base::Platform

namespace P = base::Platform;

_Check_return_ HRESULT WINAPI RoRegisterActivationFactories(
	_In_reads_(count) HSTRING* activatableClassIds,
	_In_reads_(count) PFNGETACTIVATIONFACTORY* activationFactoryCallbacks,
	_In_ UINT32 count,
	_Out_ RO_REGISTRATION_COOKIE* cookie
) {
	P::InitWRL();
	return P::RoRegisterActivationFactories
		? P::RoRegisterActivationFactories(
			activatableClassIds,
			activationFactoryCallbacks,
			count,
			cookie)
		: CO_E_DLLNOTFOUND;
}

void
WINAPI
RoRevokeActivationFactories(
	_In_ RO_REGISTRATION_COOKIE cookie
) {
	P::InitWRL();
	if (P::RoRevokeActivationFactories) {
		P::RoRevokeActivationFactories(cookie);
	}
}

extern "C" {

STDAPI
WindowsCreateString(
	_In_reads_opt_(length) PCNZWCH sourceString,
	UINT32 length,
	_Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING* string
) {
	P::InitWRL();
	return P::WindowsCreateString
		? P::WindowsCreateString(sourceString, length, string)
		: CO_E_DLLNOTFOUND;
}

STDAPI
WindowsCreateStringReference(
	_In_reads_opt_(length + 1) PCWSTR sourceString,
	UINT32 length,
	_Out_ HSTRING_HEADER* hstringHeader,
	_Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING* string
) {
	P::InitWRL();
	return P::WindowsCreateStringReference
		? P::WindowsCreateStringReference(
			sourceString,
			length,
			hstringHeader,
			string)
		: CO_E_DLLNOTFOUND;
}

STDAPI
WindowsDeleteString(
	_In_opt_ HSTRING string
) {
	P::InitWRL();
	return P::WindowsDeleteString
		? P::WindowsDeleteString(string)
		: CO_E_DLLNOTFOUND;
}

STDAPI_(PCWSTR)
WindowsGetStringRawBuffer(
		_In_opt_ HSTRING string,
		_Out_opt_ UINT32* length) {
	P::InitWRL();
	return P::WindowsGetStringRawBuffer
		? P::WindowsGetStringRawBuffer(string, length)
		: nullptr;
}

STDAPI_(BOOL)
WindowsIsStringEmpty(
	_In_opt_ HSTRING string
) {
	P::InitWRL();
	return P::WindowsIsStringEmpty && P::WindowsIsStringEmpty(string);
}

STDAPI
WindowsStringHasEmbeddedNull(
	_In_opt_ HSTRING string,
	_Out_ BOOL* hasEmbedNull
) {
	P::InitWRL();
	return P::WindowsStringHasEmbeddedNull
		? P::WindowsStringHasEmbeddedNull(string, hasEmbedNull)
		: CO_E_DLLNOTFOUND;
}

STDAPI_(BOOL)
RoOriginateErrorW(
	_In_ HRESULT error,
	_In_ UINT cchMax,
	_When_(cchMax == 0, _In_reads_or_z_opt_(MAX_ERROR_MESSAGE_CHARS))
	_When_(cchMax > 0 && cchMax < MAX_ERROR_MESSAGE_CHARS, _In_reads_or_z_(cchMax))
	_When_(cchMax >= MAX_ERROR_MESSAGE_CHARS, _In_reads_or_z_(MAX_ERROR_MESSAGE_CHARS)) PCWSTR message
) {
	P::InitWRL();
	return P::RoOriginateErrorW
		&& P::RoOriginateErrorW(error, cchMax, message);
}

STDAPI_(BOOL)
RoOriginateError(
	_In_ HRESULT error,
	_In_opt_ HSTRING message
) {
	P::InitWRL();
	return P::RoOriginateError && P::RoOriginateError(error, message);
}

} // extern "C"
