// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/qt/qt_possible_keys.h"

#include <QtGui/QKeyEvent>

#ifndef DESKTOP_APP_DISABLE_PRIVATE_QT_API
#include <private/qkeymapper_p.h>
#endif // !DESKTOP_APP_DISABLE_PRIVATE_QT_API

namespace base {

QList<PossibleKey> PossibleKeys(not_null<QKeyEvent*> e) {
#ifndef DESKTOP_APP_DISABLE_PRIVATE_QT_API
	return QKeyMapper::possibleKeys(e);
#elif QT_VERSION >= QT_VERSION_CHECK(6, 7, 0) // !DESKTOP_APP_DISABLE_PRIVATE_QT_API
	return { e->keyCombination() };
#else // DESKTOP_APP_DISABLE_PRIVATE_QT_API && Qt >= 6.7.0
	return { int(e->modifiers()) | e->key() };
#endif // DESKTOP_APP_DISABLE_PRIVATE_QT_API && Qt < 6.7.0
}

} // namespace base
