// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include <QtCore/QList>

class QKeyEvent;

namespace base {

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
using PossibleKey = QKeyCombination;
#else // Qt >= 6.7.0
using PossibleKey = int;
#endif // Qt < 6.7.0

[[nodiscard]] QList<PossibleKey> PossibleKeys(not_null<QKeyEvent*> e);

} // namespace base
