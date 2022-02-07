// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

class QString;

namespace base::Platform {

#ifdef Q_OS_MAC

bool SetCustomAppIcon(const QString &path);
bool ClearCustomAppIcon();

#else // Q_OS_MAC

inline bool SetCustomAppIcon(const QString &path) {
	return false;
}

inline bool ClearCustomAppIcon() {
	return false;
}

#endif // Q_OS_MAC

} // namespace base::Platform
