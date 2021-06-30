// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/win/base_windows_safe_library.h"

#include "base/flat_map.h"

namespace base {
namespace Platform {
namespace {

constexpr auto kMaxPathLong = 32767;

__declspec(noreturn) void FatalError(const QString &text, DWORD error = 0) {
	const auto wide = text.toStdWString();
	const auto lastError = error ? error : GetLastError();
	const auto full = lastError
		? (wide + L"\n\nError Code: " + std::to_wstring(lastError))
		: wide;
	MessageBox(nullptr, full.c_str(), L"Fatal Error", MB_ICONERROR);
	std::abort();
}

} // namespace

void CheckDynamicLibraries() {
	auto exePath = std::array<WCHAR, kMaxPathLong + 1>{ 0 };
	const auto exeLength = GetModuleFileName(
		nullptr,
		exePath.data(),
		kMaxPathLong + 1);
	if (!exeLength || exeLength >= kMaxPathLong + 1) {
		FatalError("Could not get executable path!");
	}
	const auto exe = QString::fromWCharArray(exePath.data());
	const auto last = std::max(exe.lastIndexOf('\\'), exe.lastIndexOf('/'));
	if (last < 0) {
		FatalError("Could not get executable directory!");
	}
	const auto search = (exe.mid(0, last + 1) + "*.dll").toStdWString();

	auto findData = WIN32_FIND_DATA();
	const auto findHandle = FindFirstFile(search.c_str(), &findData);
	if (findHandle == INVALID_HANDLE_VALUE) {
		const auto error = GetLastError();
		if (error != ERROR_FILE_NOT_FOUND) {
			FatalError("Could not enumerate executable path!", error);
		}
	}

	do {
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			continue;
		}
		const auto me = exe.mid(last + 1);
		FatalError("Unknown DLL library \"\
" + QString::fromWCharArray(findData.cFileName) + "\" found \
in the directory with " + me + ".\n\n\
This may be a virus or a malicious program. \n\n\
Please remove all DLL libraries from this directory:\n\n\
" + exe.midRef(0, last) + "\n\n\
Alternatively, you can move " + me + " to a new directory.");
	} while (FindNextFile(findHandle, &findData));
}

HINSTANCE SafeLoadLibrary(const QString &name, bool required) {
	static const auto Inited = [] {
		SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32);
		return true;
	}();

	const auto path = name.toStdWString();
	if (const auto result = HINSTANCE(LoadLibrary(path.c_str()))) {
		return result;
	} else if (required) {
		FatalError("Could not load required DLL '" + name + "'!");
	}
	return nullptr;
}

} // namespace Platform
} // namespace base
