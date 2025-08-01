// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/win/base_windows_winrt.h"

#include "base/debug_log.h"
#include "base/platform/win/base_windows_safe_library.h"

#include <windows.h>

namespace base::WinRT {
namespace {

int32_t(__stdcall *RoGetActivationFactory)(void* classId, winrt::guid const& iid, void** factory);
int32_t(__stdcall *RoGetAgileReference)(uint32_t options, winrt::guid const& iid, void* object, void** reference);
int32_t(__stdcall *SetThreadpoolTimerEx)(winrt::impl::ptp_timer, void*, uint32_t, uint32_t);
int32_t(__stdcall *SetThreadpoolWaitEx)(winrt::impl::ptp_wait, void*, void*, void*);
int32_t(__stdcall *RoOriginateLanguageException)(int32_t error, void* message, void* exception);
int32_t(__stdcall *RoCaptureErrorContext)(int32_t error);
void(__stdcall *RoFailFastWithErrorContext)(int32_t);
int32_t(__stdcall *RoTransformError)(int32_t, int32_t, void*);
void*(__stdcall *LoadLibraryExW)(wchar_t const* name, void* unused, uint32_t flags);
int32_t(__stdcall *FreeLibrary)(void* library);
void*(__stdcall *GetProcAddress)(void* library, char const* name);
int32_t(__stdcall *SetErrorInfo)(uint32_t reserved, void* info);
int32_t(__stdcall *GetErrorInfo)(uint32_t reserved, void** info);
int32_t(__stdcall *CoInitializeEx)(void*, uint32_t type);
void(__stdcall *CoUninitialize)();
int32_t(__stdcall *CoCreateFreeThreadedMarshaler)(void* outer, void** marshaler);
int32_t(__stdcall *CoCreateInstance)(winrt::guid const& clsid, void* outer, uint32_t context, winrt::guid const& iid, void** object);
int32_t(__stdcall *CoGetCallContext)(winrt::guid const& iid, void** object);
int32_t(__stdcall *CoGetObjectContext)(winrt::guid const& iid, void** object);
int32_t(__stdcall *CoGetApartmentType)(int32_t* type, int32_t* qualifier);
void*(__stdcall *CoTaskMemAlloc)(std::size_t size);
void(__stdcall *CoTaskMemFree)(void* ptr);
winrt::impl::bstr(__stdcall *SysAllocString)(wchar_t const* value);
void(__stdcall *SysFreeString)(winrt::impl::bstr string);
uint32_t(__stdcall *SysStringLen)(winrt::impl::bstr string);
int32_t(__stdcall *IIDFromString)(wchar_t const* string, winrt::guid* iid);
int32_t(__stdcall *MultiByteToWideChar)(uint32_t codepage, uint32_t flags, char const* in_string, int32_t in_size, wchar_t* out_string, int32_t out_size);
int32_t(__stdcall *WideCharToMultiByte)(uint32_t codepage, uint32_t flags, wchar_t const* int_string, int32_t in_size, char* out_string, int32_t out_size, char const* default_char, int32_t* default_used);
void*(__stdcall *HeapAlloc)(void* heap, uint32_t flags, size_t bytes);
int32_t(__stdcall *HeapFree)(void* heap, uint32_t flags, void* value);
void*(__stdcall *GetProcessHeap)();
uint32_t(__stdcall *FormatMessageW)(uint32_t flags, void const* source, uint32_t code, uint32_t language, wchar_t* buffer, uint32_t size, va_list* arguments);
uint32_t(__stdcall *GetLastError)();
void(__stdcall *GetSystemTimePreciseAsFileTime)(void* result);
uintptr_t(__stdcall *VirtualQuery)(void* address, void* buffer, uintptr_t length);
void*(__stdcall *EncodePointer)(void* ptr);
int32_t(__stdcall *OpenProcessToken)(void* process, uint32_t access, void** token);
void*(__stdcall *GetCurrentProcess)();
int32_t(__stdcall *DuplicateToken)(void* existing, uint32_t level, void** duplicate);
int32_t(__stdcall *OpenThreadToken)(void* thread, uint32_t access, int32_t self, void** token);
void*(__stdcall *GetCurrentThread)();
int32_t(__stdcall *SetThreadToken)(void** thread, void* token);
void(__stdcall *AcquireSRWLockExclusive)(winrt::impl::srwlock* lock);
void(__stdcall *AcquireSRWLockShared)(winrt::impl::srwlock* lock);
uint8_t(__stdcall *TryAcquireSRWLockExclusive)(winrt::impl::srwlock* lock);
uint8_t(__stdcall *TryAcquireSRWLockShared)(winrt::impl::srwlock* lock);
void(__stdcall *ReleaseSRWLockExclusive)(winrt::impl::srwlock* lock);
void(__stdcall *ReleaseSRWLockShared)(winrt::impl::srwlock* lock);
int32_t(__stdcall *SleepConditionVariableSRW)(winrt::impl::condition_variable* cv, winrt::impl::srwlock* lock, uint32_t milliseconds, uint32_t flags);
void(__stdcall *WakeConditionVariable)(winrt::impl::condition_variable* cv);
void(__stdcall *WakeAllConditionVariable)(winrt::impl::condition_variable* cv);
void*(__stdcall *InterlockedPushEntrySList)(void* head, void* entry);
void*(__stdcall *InterlockedFlushSList)(void* head);
void*(__stdcall *CreateEventW)(void*, int32_t, int32_t, void*);
int32_t(__stdcall *SetEvent)(void*);
int32_t(__stdcall *CloseHandle)(void* hObject);
uint32_t(__stdcall *WaitForSingleObject)(void* handle, uint32_t milliseconds);
int32_t(__stdcall *TrySubmitThreadpoolCallback)(void(__stdcall *callback)(void*, void* context), void* context, void*);
winrt::impl::ptp_timer(__stdcall *CreateThreadpoolTimer)(void(__stdcall *callback)(void*, void* context, void*), void* context, void*);
void(__stdcall *SetThreadpoolTimer)(winrt::impl::ptp_timer timer, void* time, uint32_t period, uint32_t window);
void(__stdcall *CloseThreadpoolTimer)(winrt::impl::ptp_timer timer);
winrt::impl::ptp_wait(__stdcall *CreateThreadpoolWait)(void(__stdcall *callback)(void*, void* context, void*, uint32_t result), void* context, void*);
void(__stdcall *SetThreadpoolWait)(winrt::impl::ptp_wait wait, void* handle, void* timeout);
void(__stdcall *CloseThreadpoolWait)(winrt::impl::ptp_wait wait);
winrt::impl::ptp_io(__stdcall *CreateThreadpoolIo)(void* object, void(__stdcall *callback)(void*, void* context, void* overlapped, uint32_t result, std::size_t bytes, void*) noexcept, void* context, void*);
void(__stdcall *StartThreadpoolIo)(winrt::impl::ptp_io io);
void(__stdcall *CancelThreadpoolIo)(winrt::impl::ptp_io io);
void(__stdcall *CloseThreadpoolIo)(winrt::impl::ptp_io io);
winrt::impl::ptp_pool(__stdcall *CreateThreadpool)(void* reserved);
void(__stdcall *SetThreadpoolThreadMaximum)(winrt::impl::ptp_pool pool, uint32_t value);
int32_t(__stdcall *SetThreadpoolThreadMinimum)(winrt::impl::ptp_pool pool, uint32_t value);
void(__stdcall *CloseThreadpool)(winrt::impl::ptp_pool pool);

[[nodiscard]] bool Resolve() {
	const auto ole32 = Platform::SafeLoadLibrary(L"ole32.dll");
	const auto oleaut32 = Platform::SafeLoadLibrary(L"oleaut32.dll");
	const auto combase = Platform::SafeLoadLibrary(L"combase.dll");
	const auto kernel32 = Platform::SafeLoadLibrary(L"kernel32.dll");
	const auto advapi32 = Platform::SafeLoadLibrary(L"advapi32.dll");
	auto total = 0, resolved = 0;
#define LOAD_SYMBOL(lib, name) ++total; if (Platform::LoadMethod(lib, #name, name)) ++resolved;
	LOAD_SYMBOL(combase, RoGetActivationFactory);
	LOAD_SYMBOL(combase, RoGetAgileReference);
	LOAD_SYMBOL(kernel32, SetThreadpoolTimerEx);
	LOAD_SYMBOL(kernel32, SetThreadpoolWaitEx);
	LOAD_SYMBOL(combase, RoOriginateLanguageException);
	LOAD_SYMBOL(combase, RoCaptureErrorContext);
	LOAD_SYMBOL(combase, RoFailFastWithErrorContext);
	LOAD_SYMBOL(combase, RoTransformError);
	LOAD_SYMBOL(kernel32, LoadLibraryExW);
	LOAD_SYMBOL(kernel32, FreeLibrary);
	LOAD_SYMBOL(kernel32, GetProcAddress);
	LOAD_SYMBOL(oleaut32, SetErrorInfo);
	LOAD_SYMBOL(oleaut32, GetErrorInfo);
	LOAD_SYMBOL(ole32, CoInitializeEx);
	LOAD_SYMBOL(ole32, CoUninitialize);
	LOAD_SYMBOL(ole32, CoCreateFreeThreadedMarshaler);
	LOAD_SYMBOL(ole32, CoCreateInstance);
	LOAD_SYMBOL(ole32, CoGetCallContext);
	LOAD_SYMBOL(ole32, CoGetObjectContext);
	LOAD_SYMBOL(ole32, CoGetApartmentType);
	LOAD_SYMBOL(ole32, CoTaskMemAlloc);
	LOAD_SYMBOL(ole32, CoTaskMemFree);
	LOAD_SYMBOL(oleaut32, SysAllocString);
	LOAD_SYMBOL(oleaut32, SysFreeString);
	LOAD_SYMBOL(oleaut32, SysStringLen);
	LOAD_SYMBOL(ole32, IIDFromString);
	LOAD_SYMBOL(kernel32, MultiByteToWideChar);
	LOAD_SYMBOL(kernel32, WideCharToMultiByte);
	LOAD_SYMBOL(kernel32, HeapAlloc);
	LOAD_SYMBOL(kernel32, HeapFree);
	LOAD_SYMBOL(kernel32, GetProcessHeap);
	LOAD_SYMBOL(kernel32, FormatMessageW);
	LOAD_SYMBOL(kernel32, GetLastError);
	LOAD_SYMBOL(kernel32, GetSystemTimePreciseAsFileTime);
	LOAD_SYMBOL(kernel32, VirtualQuery);
	LOAD_SYMBOL(kernel32, EncodePointer);
	LOAD_SYMBOL(kernel32, OpenProcessToken);
	LOAD_SYMBOL(kernel32, GetCurrentProcess);
	LOAD_SYMBOL(advapi32, DuplicateToken);
	LOAD_SYMBOL(advapi32, OpenThreadToken);
	LOAD_SYMBOL(kernel32, GetCurrentThread);
	LOAD_SYMBOL(kernel32, SetThreadToken);
	LOAD_SYMBOL(kernel32, AcquireSRWLockExclusive);
	LOAD_SYMBOL(kernel32, AcquireSRWLockShared);
	LOAD_SYMBOL(kernel32, TryAcquireSRWLockExclusive);
	LOAD_SYMBOL(kernel32, TryAcquireSRWLockShared);
	LOAD_SYMBOL(kernel32, ReleaseSRWLockExclusive);
	LOAD_SYMBOL(kernel32, ReleaseSRWLockShared);
	LOAD_SYMBOL(kernel32, SleepConditionVariableSRW);
	LOAD_SYMBOL(kernel32, WakeConditionVariable);
	LOAD_SYMBOL(kernel32, WakeAllConditionVariable);
	LOAD_SYMBOL(kernel32, InterlockedPushEntrySList);
	LOAD_SYMBOL(kernel32, InterlockedFlushSList);
	LOAD_SYMBOL(kernel32, CreateEventW);
	LOAD_SYMBOL(kernel32, SetEvent);
	LOAD_SYMBOL(kernel32, CloseHandle);
	LOAD_SYMBOL(kernel32, WaitForSingleObject);
	LOAD_SYMBOL(kernel32, TrySubmitThreadpoolCallback);
	LOAD_SYMBOL(kernel32, CreateThreadpoolTimer);
	LOAD_SYMBOL(kernel32, SetThreadpoolTimer);
	LOAD_SYMBOL(kernel32, CloseThreadpoolTimer);
	LOAD_SYMBOL(kernel32, CreateThreadpoolWait);
	LOAD_SYMBOL(kernel32, SetThreadpoolWait);
	LOAD_SYMBOL(kernel32, CloseThreadpoolWait);
	LOAD_SYMBOL(kernel32, CreateThreadpoolIo);
	LOAD_SYMBOL(kernel32, StartThreadpoolIo);
	LOAD_SYMBOL(kernel32, CancelThreadpoolIo);
	LOAD_SYMBOL(kernel32, CloseThreadpoolIo);
	LOAD_SYMBOL(kernel32, CreateThreadpool);
	LOAD_SYMBOL(kernel32, SetThreadpoolThreadMaximum);
	LOAD_SYMBOL(kernel32, SetThreadpoolThreadMinimum);
	LOAD_SYMBOL(kernel32, CloseThreadpool);
#undef LOAD_SYMBOL
	return (total == resolved);
}

} // namespace

[[nodiscard]] bool Supported() {
	static const auto Result = Resolve();
	return Result;
}

} // namespace base::WinRT

namespace P = base::WinRT;

extern "C" {

int32_t __stdcall WINRT_IMPL_RoGetActivationFactory(void* classId, winrt::guid const& iid, void** factory) noexcept {
	return P::RoGetActivationFactory(classId, iid, factory);
}

int32_t __stdcall WINRT_IMPL_RoGetAgileReference(uint32_t options, winrt::guid const& iid, void* object, void** reference) noexcept {
	return P::RoGetAgileReference(options, iid, object, reference);
}

int32_t __stdcall WINRT_IMPL_SetThreadpoolTimerEx(winrt::impl::ptp_timer timer, void *time, uint32_t period, uint32_t window) noexcept {
	return P::SetThreadpoolTimerEx(timer, time, period, window);
}

int32_t __stdcall WINRT_IMPL_SetThreadpoolWaitEx(winrt::impl::ptp_wait wait, void *handle, void *timeout, void *reserved) noexcept {
	return P::SetThreadpoolWaitEx(wait, handle, timeout, reserved);
}

int32_t __stdcall WINRT_IMPL_RoOriginateLanguageException(int32_t error, void* message, void* exception) noexcept {
	return P::RoOriginateLanguageException
		? P::RoOriginateLanguageException(error, message, exception)
		: TRUE;
}

int32_t __stdcall WINRT_IMPL_RoCaptureErrorContext(int32_t error) noexcept {
	return P::RoCaptureErrorContext(error);
}

void __stdcall WINRT_IMPL_RoFailFastWithErrorContext(int32_t error) noexcept {
	return P::RoFailFastWithErrorContext(error);
}

int32_t __stdcall WINRT_IMPL_RoTransformError(int32_t error, int32_t transform, void *context) noexcept {
	return P::RoTransformError(error, transform, context);
}

void* __stdcall WINRT_IMPL_LoadLibraryExW(wchar_t const* name, void* unused, uint32_t flags) noexcept {
	return P::LoadLibraryExW(name, unused, flags);
}

int32_t __stdcall WINRT_IMPL_FreeLibrary(void* library) noexcept {
	return P::FreeLibrary(library);
}

void* __stdcall WINRT_IMPL_GetProcAddress(void* library, char const* name) noexcept {
	return P::GetProcAddress(library, name);
}

int32_t __stdcall WINRT_IMPL_SetErrorInfo(uint32_t reserved, void* info) noexcept {
	return P::SetErrorInfo(reserved, info);
}

int32_t __stdcall WINRT_IMPL_GetErrorInfo(uint32_t reserved, void** info) noexcept {
	return P::GetErrorInfo(reserved, info);
}
int32_t __stdcall WINRT_IMPL_CoInitializeEx(void *reserved, uint32_t type) noexcept {
	return P::CoInitializeEx(reserved, type);
}

int32_t __stdcall WINRT_IMPL_CoCreateFreeThreadedMarshaler(void* outer, void** marshaler) noexcept {
	return P::CoCreateFreeThreadedMarshaler(outer, marshaler);
}

int32_t __stdcall WINRT_IMPL_CoCreateInstance(winrt::guid const& clsid, void* outer, uint32_t context, winrt::guid const& iid, void** object) noexcept {
	return P::CoCreateInstance(clsid, outer, context, iid, object);
}

int32_t __stdcall WINRT_IMPL_CoGetCallContext(winrt::guid const& iid, void** object) noexcept {
	return P::CoGetCallContext(iid, object);
}

int32_t __stdcall WINRT_IMPL_CoGetApartmentType(int32_t* type, int32_t* qualifier) noexcept {
	return P::CoGetApartmentType(type, qualifier);
}

void* __stdcall WINRT_IMPL_CoTaskMemAlloc(std::size_t size) noexcept {
	return P::CoTaskMemAlloc(size);
}
void __stdcall WINRT_IMPL_CoTaskMemFree(void* ptr) noexcept {
	return P::CoTaskMemFree(ptr);
}

void __stdcall WINRT_IMPL_SysFreeString(winrt::impl::bstr string) noexcept {
	return P::SysFreeString(string);
}

uint32_t __stdcall WINRT_IMPL_SysStringLen(winrt::impl::bstr string) noexcept {
	return P::SysStringLen(string);
}
int32_t __stdcall WINRT_IMPL_IIDFromString(wchar_t const* string, winrt::guid* iid) noexcept {
	return P::IIDFromString(string, iid);
}

int32_t __stdcall WINRT_IMPL_MultiByteToWideChar(uint32_t codepage, uint32_t flags, char const* in_string, int32_t in_size, wchar_t* out_string, int32_t out_size) noexcept {
	return P::MultiByteToWideChar(codepage, flags, in_string, in_size, out_string, out_size);
}

int32_t __stdcall WINRT_IMPL_WideCharToMultiByte(uint32_t codepage, uint32_t flags, wchar_t const* int_string, int32_t in_size, char* out_string, int32_t out_size, char const* default_char, int32_t* default_used) noexcept {
	return P::WideCharToMultiByte(codepage, flags, int_string, in_size, out_string, out_size, default_char, default_used);
}

int32_t __stdcall WINRT_IMPL_HeapFree(void* heap, uint32_t flags, void* value) noexcept {
	return P::HeapFree(heap, flags, value);
}

void* __stdcall WINRT_IMPL_GetProcessHeap() noexcept {
	return P::GetProcessHeap();
}

uint32_t __stdcall WINRT_IMPL_FormatMessageW(uint32_t flags, void const* source, uint32_t code, uint32_t language, wchar_t* buffer, uint32_t size, va_list* arguments) noexcept {
	return P::FormatMessageW(flags, source, code, language, buffer, size, arguments);
}

uint32_t __stdcall WINRT_IMPL_GetLastError() noexcept {
	return P::GetLastError();
}

int32_t __stdcall WINRT_IMPL_OpenProcessToken(void* process, uint32_t access, void** token) noexcept {
	return P::OpenProcessToken(process, access, token);
}

void* __stdcall WINRT_IMPL_GetCurrentProcess() noexcept {
	return P::GetCurrentProcess();
}

int32_t __stdcall WINRT_IMPL_DuplicateToken(void* existing, uint32_t level, void** duplicate) noexcept {
	return P::DuplicateToken(existing, level, duplicate);
}

int32_t __stdcall WINRT_IMPL_OpenThreadToken(void* thread, uint32_t access, int32_t self, void** token) noexcept {
	return P::OpenThreadToken(thread, access, self, token);
}

void __stdcall WINRT_IMPL_AcquireSRWLockExclusive(winrt::impl::srwlock* lock) noexcept {
	return P::AcquireSRWLockExclusive(lock);
}

void __stdcall WINRT_IMPL_AcquireSRWLockShared(winrt::impl::srwlock* lock) noexcept {
	return P::AcquireSRWLockShared(lock);
}

uint8_t __stdcall WINRT_IMPL_TryAcquireSRWLockExclusive(winrt::impl::srwlock* lock) noexcept {
	return P::TryAcquireSRWLockExclusive(lock);
}

uint8_t __stdcall WINRT_IMPL_TryAcquireSRWLockShared(winrt::impl::srwlock* lock) noexcept {
	return P::TryAcquireSRWLockShared(lock);
}

void __stdcall WINRT_IMPL_ReleaseSRWLockExclusive(winrt::impl::srwlock* lock) noexcept {
	return P::ReleaseSRWLockExclusive(lock);
}

void __stdcall WINRT_IMPL_ReleaseSRWLockShared(winrt::impl::srwlock* lock) noexcept {
	return P::ReleaseSRWLockShared(lock);
}

void __stdcall WINRT_IMPL_WakeConditionVariable(winrt::impl::condition_variable* cv) noexcept {
	return P::WakeConditionVariable(cv);
}

void __stdcall WINRT_IMPL_WakeAllConditionVariable(winrt::impl::condition_variable* cv) noexcept {
	return P::WakeAllConditionVariable(cv);
}

void* __stdcall WINRT_IMPL_InterlockedPushEntrySList(void* head, void* entry) noexcept {
	return P::InterlockedPushEntrySList(head, entry);
}

void* __stdcall WINRT_IMPL_CreateEventW(void *event, int32_t manual, int32_t initial, void *name) noexcept {
	return P::CreateEventW(event, manual, initial, name);
}

int32_t __stdcall WINRT_IMPL_SetEvent(void *event) noexcept {
	return P::SetEvent(event);
}

int32_t __stdcall WINRT_IMPL_CloseHandle(void* hObject) noexcept {
	return P::CloseHandle(hObject);
}

uint32_t __stdcall WINRT_IMPL_WaitForSingleObject(void* handle, uint32_t milliseconds) noexcept {
	return P::WaitForSingleObject(handle, milliseconds);
}

int32_t __stdcall WINRT_IMPL_TrySubmitThreadpoolCallback(void(__stdcall *callback)(void*, void* context), void* context, void*) noexcept {
	return P::TrySubmitThreadpoolCallback(callback, context, context);
}

winrt::impl::ptp_timer __stdcall WINRT_IMPL_CreateThreadpoolTimer(void(__stdcall *callback)(void*, void* context, void*), void* context, void*) noexcept {
	return P::CreateThreadpoolTimer(callback, context, context);
}

void __stdcall WINRT_IMPL_SetThreadpoolTimer(winrt::impl::ptp_timer timer, void* time, uint32_t period, uint32_t window) noexcept {
	return P::SetThreadpoolTimer(timer, time, period, window);
}

winrt::impl::ptp_wait __stdcall WINRT_IMPL_CreateThreadpoolWait(void(__stdcall *callback)(void*, void* context, void*, uint32_t result), void* context, void*) noexcept {
	return P::CreateThreadpoolWait(callback, context, context);
}

void __stdcall WINRT_IMPL_SetThreadpoolWait(winrt::impl::ptp_wait wait, void* handle, void* timeout) noexcept {
	return P::SetThreadpoolWait(wait, handle, timeout);
}

winrt::impl::ptp_io __stdcall WINRT_IMPL_CreateThreadpoolIo(void* object, void(__stdcall *callback)(void*, void* context, void* overlapped, uint32_t result, std::size_t bytes, void*) noexcept, void* context, void*) noexcept {
	return P::CreateThreadpoolIo(object, callback, context, context);
}

void __stdcall WINRT_IMPL_StartThreadpoolIo(winrt::impl::ptp_io io) noexcept {
	return P::StartThreadpoolIo(io);
}

void __stdcall WINRT_IMPL_CancelThreadpoolIo(winrt::impl::ptp_io io) noexcept {
	return P::CancelThreadpoolIo(io);
}

winrt::impl::ptp_pool __stdcall WINRT_IMPL_CreateThreadpool(void* reserved) noexcept {
	return P::CreateThreadpool(reserved);
}

void __stdcall WINRT_IMPL_SetThreadpoolThreadMaximum(winrt::impl::ptp_pool pool, uint32_t value) noexcept {
	return P::SetThreadpoolThreadMaximum(pool, value);
}

int32_t __stdcall WINRT_IMPL_SetThreadpoolThreadMinimum(winrt::impl::ptp_pool pool, uint32_t value) noexcept {
	return P::SetThreadpoolThreadMinimum(pool, value);
}

void __stdcall WINRT_IMPL_CloseThreadpool(winrt::impl::ptp_pool pool) noexcept {
	return P::CloseThreadpool(pool);
}

} // extern "C"
