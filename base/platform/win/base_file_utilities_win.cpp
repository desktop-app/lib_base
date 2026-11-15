// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/win/base_file_utilities_win.h"

#include "base/platform/win/base_windows_safe_library.h"
#include "base/platform/win/base_windows_shlobj_h.h"
#include "base/algorithm.h"
#include "base/custom_delete.h"
#include "base/flat_map.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStringList>
#include <QtGui/QDesktopServices>

#include <array>
#include <string>
#include <shellapi.h>
#include <Shlwapi.h>
#include <RestartManager.h>
#include <io.h>

#define LOAD_SYMBOL(lib, name) ::base::Platform::LoadMethod(lib, #name, name)

namespace base::Platform {
namespace {

// RSTRTMGR.DLL

DWORD(__stdcall *RmStartSession)(
	_Out_ DWORD *pSessionHandle,
	_Reserved_ DWORD dwSessionFlags,
	_Out_writes_(CCH_RM_SESSION_KEY + 1) WCHAR strSessionKey[]);
DWORD(__stdcall *RmRegisterResources)(
	_In_ DWORD dwSessionHandle,
	_In_ UINT nFiles,
	_In_reads_opt_(nFiles) LPCWSTR rgsFileNames[],
	_In_ UINT nApplications,
	_In_reads_opt_(nApplications) RM_UNIQUE_PROCESS rgApplications[],
	_In_ UINT nServices,
	_In_reads_opt_(nServices) LPCWSTR rgsServiceNames[]);
DWORD(__stdcall *RmGetList)(
	_In_ DWORD dwSessionHandle,
	_Out_ UINT *pnProcInfoNeeded,
	_Inout_ UINT *pnProcInfo,
	_Inout_updates_opt_(*pnProcInfo) RM_PROCESS_INFO rgAffectedApps[],
	_Out_ LPDWORD lpdwRebootReasons);
DWORD(__stdcall *RmShutdown)(
	_In_ DWORD dwSessionHandle,
	_In_ ULONG lActionFlags,
	_In_opt_ RM_WRITE_STATUS_CALLBACK fnStatus);
DWORD(__stdcall *RmEndSession)(
	_In_ DWORD dwSessionHandle);

} // namespace

void ShowInFolder(const QStringList &filepaths) {
	auto folders = base::flat_map<QString, QStringList>();
	for (const auto &filepath : filepaths) {
		folders[QFileInfo(filepath).absolutePath()].push_back(filepath);
	}

	const auto initialized = SUCCEEDED(CoInitializeEx(
		nullptr,
		COINIT_APARTMENTTHREADED));
	const auto uninitialize = gsl::finally([=] {
		if (initialized) {
			CoUninitialize();
		}
	});
	using ShellId = std::unique_ptr<
		ITEMIDLIST_ABSOLUTE,
		base::custom_delete<ILFree>>;
	const auto shellId = [](const QString &path) {
		const auto native = QDir::toNativeSeparators(path).toStdWString();
		return ShellId(ILCreateFromPathW(native.c_str()));
	};
	for (const auto &[folderPath, paths] : folders) {
		const auto folder = shellId(folderPath);
		auto items = std::vector<ShellId>();
		auto children = std::vector<PCUITEMID_CHILD>();
		for (const auto &path : paths) {
			if (auto id = shellId(path)) {
				children.push_back(ILFindLastID(id.get()));
				items.push_back(std::move(id));
			}
		}
		if (folder && !children.empty()) {
			SHOpenFolderAndSelectItems(
				folder.get(),
				UINT(children.size()),
				children.data(),
				0);
		} else if (paths.size() == 1) {
			const auto native = QDir::toNativeSeparators(paths.front());
			const auto command = (u"/select,\""_q + native + '"').toStdWString();
			ShellExecute(
				nullptr,
				nullptr,
				L"explorer",
				command.c_str(),
				nullptr,
				SW_SHOWNORMAL);
		} else {
			QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
		}
	}
}

QString FileNameFromUserString(QString name) {
	const auto kBadExtensions = { qstr(".lnk"), qstr(".scf") };
	const auto kMaskExtension = qstr(".download");
	for (const auto extension : kBadExtensions) {
		if (name.endsWith(extension, Qt::CaseInsensitive)) {
			name += kMaskExtension;
		}
	}

	static const auto BadNames = {
		qstr("CON"),
		qstr("PRN"),
		qstr("AUX"),
		qstr("NUL"),
		qstr("COM1"),
		qstr("COM2"),
		qstr("COM3"),
		qstr("COM4"),
		qstr("COM5"),
		qstr("COM6"),
		qstr("COM7"),
		qstr("COM8"),
		qstr("COM9"),
		qstr("LPT1"),
		qstr("LPT2"),
		qstr("LPT3"),
		qstr("LPT4"),
		qstr("LPT5"),
		qstr("LPT6"),
		qstr("LPT7"),
		qstr("LPT8"),
		qstr("LPT9")
	};
	for (const auto bad : BadNames) {
		if (name.startsWith(bad, Qt::CaseInsensitive)) {
			if (name.size() == bad.size() || name[bad.size()] == '.') {
				name = '_' + name;
				break;
			}
		}
	}
	return name;
}

bool DeleteDirectory(QString path) {
	if (path.endsWith('/')) {
		path.chop(1);
	}
	const auto wide = QDir::toNativeSeparators(path).toStdWString()
		+ wchar_t(0)
		+ wchar_t(0);
	SHFILEOPSTRUCT file_op = {
		NULL,
		FO_DELETE,
		wide.data(),
		L"",
		FOF_NOCONFIRMATION |
		FOF_NOERRORUI |
		FOF_SILENT,
		false,
		0,
		L""
	};
	return (SHFileOperation(&file_op) == 0);
}

void RemoveQuarantine(const QString &path) {
}

QString BundledResourcesPath() {
	Unexpected("BundledResourcesPath not implemented.");
}

QString CurrentExecutablePath(int argc, char *argv[]) {
	auto result = std::array<WCHAR, MAX_PATH + 1>{ 0 };
	const auto count = GetModuleFileName(
		nullptr,
		result.data(),
		MAX_PATH + 1);
	if (count < MAX_PATH + 1) {
		const auto info = QFileInfo(QDir::fromNativeSeparators(
			QString::fromWCharArray(result.data(), count)));
		return info.absoluteFilePath();
	}

	// Fallback to the first command line argument.
	auto argsCount = 0;
	if (const auto args = CommandLineToArgvW(GetCommandLine(), &argsCount)) {
		auto info = QFileInfo(QDir::fromNativeSeparators(
			QString::fromWCharArray(args[0])));
		LocalFree(args);
		return info.absoluteFilePath();
	}
	return QString();
}

bool CloseProcesses(const QString &filename) {
	static const auto loaded = [&] {
		const auto LibRstrtMgr = SafeLoadLibrary(L"rstrtmgr.dll");
		return LOAD_SYMBOL(LibRstrtMgr, RmStartSession)
			&& LOAD_SYMBOL(LibRstrtMgr, RmRegisterResources)
			&& LOAD_SYMBOL(LibRstrtMgr, RmGetList)
			&& LOAD_SYMBOL(LibRstrtMgr, RmShutdown)
			&& LOAD_SYMBOL(LibRstrtMgr, RmEndSession);
	}();
	if (!loaded) {
		return false;
	}

	auto session = DWORD();
	auto sessionKey = std::wstring(CCH_RM_SESSION_KEY + 1, wchar_t(0));
	auto error = RmStartSession(&session, 0, sessionKey.data());
	if (error != ERROR_SUCCESS) {
		return false;
	}
	const auto guard = gsl::finally([&] { RmEndSession(session); });

	const auto path = QDir::toNativeSeparators(filename).toStdWString();
	auto nullterm = path.c_str();
	error = RmRegisterResources(
		session,
		1,
		&nullterm,
		0,
		nullptr,
		0,
		nullptr);
	if (error != ERROR_SUCCESS) {
		return false;
	}

	auto processInfoNeeded = UINT(0);
	auto processInfoCount = UINT(0);
	auto reason = DWORD();

	error = RmGetList(
		session,
		&processInfoNeeded,
		&processInfoCount,
		nullptr,
		&reason);
	if (error != ERROR_SUCCESS && error != ERROR_MORE_DATA) {
		return false;
	} else if (processInfoNeeded <= 0) {
		return true;
	}
	error = RmShutdown(session, RmForceShutdown, NULL);
	if (error != ERROR_SUCCESS) {
		return false;
	}
	return true;
}

bool RenameWithOverwrite(const QString &from, const QString &to) {
	const auto fromPath = QDir::toNativeSeparators(from).toStdWString();
	const auto toPath = QDir::toNativeSeparators(to).toStdWString();
	return MoveFileEx(
		fromPath.c_str(),
		toPath.c_str(),
		MOVEFILE_REPLACE_EXISTING);
}

void FlushFileData(QFile &file) {
	file.flush();
	if (const auto descriptor = file.handle()) {
		if (const auto handle = HANDLE(_get_osfhandle(descriptor))) {
			FlushFileBuffers(handle);
		}
	}
}

} // namespace base::Platform
