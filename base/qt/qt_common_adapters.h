// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtGui/QInputDevice>
#else // Qt >= 6.0.0
#include <QtGui/QTouchDevice>
#endif // Qt < 6.0.0

namespace base {

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
using TouchDevice = QInputDevice::DeviceType;
#else // Qt >= 6.0.0
using TouchDevice = QTouchDevice;
#endif // Qt < 6.0.0

} // namespace base
