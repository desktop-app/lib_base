// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/win/base_windows_winrt.h"

#include "base/platform/win/base_windows_safe_library.h"

#include <windows.h>

namespace base::WinRT {
namespace {

int32_t(__stdcall *CoIncrementMTAUsage)(void** cookie);
int32_t(__stdcall *RoInitialize)(uint32_t type);
int32_t(__stdcall *GetRestrictedErrorInfo)(void** info);
int32_t(__stdcall *RoGetActivationFactory)(void* classId, winrt::guid const& iid, void** factory);
int32_t(__stdcall *RoOriginateLanguageException)(int32_t error, void* message, void* exception);
int32_t(__stdcall *SetRestrictedErrorInfo)(void* info);
int32_t(__stdcall *WindowsCreateString)(wchar_t const* sourceString, uint32_t length, void** string);
int32_t(__stdcall *WindowsCreateStringReference)(wchar_t const* sourceString, uint32_t length, void* hstringHeader, void** string);
int32_t(__stdcall *WindowsDuplicateString)(void* string, void** newString);
int32_t(__stdcall *WindowsDeleteString)(void* string);
int32_t(__stdcall *WindowsPreallocateStringBuffer)(uint32_t length, wchar_t** charBuffer, void** bufferHandle);
int32_t(__stdcall *WindowsDeleteStringBuffer)(void* bufferHandle);
int32_t(__stdcall *WindowsPromoteStringBuffer)(void* bufferHandle, void** string);
wchar_t const*(__stdcall *WindowsGetStringRawBuffer)(void* string, uint32_t* length);

[[nodiscard]] bool Resolve() {
#define LOAD_SYMBOL(lib, name) Platform::LoadMethod(lib, #name, name)
	const auto ole32 = Platform::SafeLoadLibrary(L"ole32.dll");
	const auto combase = Platform::SafeLoadLibrary(L"combase.dll");
	return LOAD_SYMBOL(ole32, CoIncrementMTAUsage)
		&& LOAD_SYMBOL(combase, RoInitialize)
		&& LOAD_SYMBOL(combase, GetRestrictedErrorInfo)
		&& LOAD_SYMBOL(combase, RoGetActivationFactory)
		&& LOAD_SYMBOL(combase, RoOriginateLanguageException)
		&& LOAD_SYMBOL(combase, SetRestrictedErrorInfo)
		&& LOAD_SYMBOL(combase, WindowsCreateString)
		&& LOAD_SYMBOL(combase, WindowsCreateStringReference)
		&& LOAD_SYMBOL(combase, WindowsDuplicateString)
		&& LOAD_SYMBOL(combase, WindowsDeleteString)
		&& LOAD_SYMBOL(combase, WindowsPreallocateStringBuffer)
		&& LOAD_SYMBOL(combase, WindowsDeleteStringBuffer)
		&& LOAD_SYMBOL(combase, WindowsPromoteStringBuffer)
		&& LOAD_SYMBOL(combase, WindowsGetStringRawBuffer);
#undef LOAD_SYMBOL
}

} // namespace

[[nodiscard]] bool Supported() {
	static const auto Result = Resolve();
	return Result;
}

} // namespace base::WinRT

namespace P = base::WinRT;

extern "C" {

int32_t __stdcall WINRT_IMPL_RoOriginateLanguageException(int32_t error, void* message, void* exception) noexcept {
	return P::RoOriginateLanguageException(error, message, exception);
}

} // extern "C"
