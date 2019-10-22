// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/file_lock.h"

#include "base/platform/win/base_windows_h.h"

#include <QtCore/QDir>
#include <io.h>
#include <fileapi.h>
#include <RestartManager.h>

namespace base {
namespace {

bool CloseProcesses(const QString &filename) {
	auto result = BOOL(FALSE);
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

} // namespace

class FileLock::Lock {
public:
	static int Acquire(const QFile &file);

	explicit Lock(int descriptor);
	~Lock();

private:
	static constexpr auto offsetLow = DWORD(kLockOffset);
	static constexpr auto offsetHigh = DWORD(0);
	static constexpr auto limitLow = DWORD(kLockLimit);
	static constexpr auto limitHigh = DWORD(0);

	int _descriptor = 0;

};

int FileLock::Lock::Acquire(const QFile &file) {
	const auto descriptor = file.handle();
	if (!descriptor || !file.isOpen()) {
		return false;
	}
	const auto handle = HANDLE(_get_osfhandle(descriptor));
	if (!handle) {
		return false;
	}
	return LockFile(handle, offsetLow, offsetHigh, limitLow, limitHigh)
		? descriptor
		: 0;
}

FileLock::Lock::Lock(int descriptor) : _descriptor(descriptor) {
}

FileLock::Lock::~Lock() {
	if (const auto handle = HANDLE(_get_osfhandle(_descriptor))) {
		UnlockFile(handle, offsetLow, offsetHigh, limitLow, limitHigh);
	}
}

FileLock::FileLock() = default;

bool FileLock::lock(QFile &file, QIODevice::OpenMode mode) {
	Expects(_lock == nullptr || file.isOpen());

	unlock();
	file.close();
	do {
		if (!file.open(mode)) {
			return false;
		} else if (const auto descriptor = Lock::Acquire(file)) {
			_lock = std::make_unique<Lock>(descriptor);
			return true;
		}
		file.close();
	} while (CloseProcesses(file.fileName()));

	return false;
}

bool FileLock::locked() const {
	return (_lock != nullptr);
}

void FileLock::unlock() {
	_lock = nullptr;
}

FileLock::~FileLock() = default;

} // namespace base
