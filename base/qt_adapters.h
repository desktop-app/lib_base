// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/algorithm.h"

#include <QtGui/QScreen>
#include <QtGui/QImage>
#include <QtGui/QGuiApplication>
#include <QtNetwork/QLocalSocket>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QString>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtGui/QInputDevice>
#else // Qt >= 6.0.0
#include <QtGui/QTouchDevice>
#endif // Qt < 6.0.0

namespace base {
namespace {

struct ContainerImplHelper {
    enum CutResult { Null, Empty, Full, Subset };
    static constexpr CutResult mid(qsizetype originalLength, qsizetype *_position, qsizetype *_length) {
		qsizetype &position = *_position;
		qsizetype &length = *_length;
		if (position > originalLength) {
			position = 0;
			length = 0;
			return Null;
		}

		if (position < 0) {
			if (length < 0 || length + position >= originalLength) {
				position = 0;
				length = originalLength;
				return Full;
			}
			if (length + position <= 0) {
				position = length = 0;
				return Null;
			}
			length += position;
			position = 0;
		} else if (size_t(length) > size_t(originalLength - position)) {
			length = originalLength - position;
		}

		if (position == 0 && length == originalLength)
			return Full;

		return length > 0 ? Subset : Empty;
	}
};

} // namespace

using QLocalSocketErrorSignal = void(QLocalSocket::*)(QLocalSocket::LocalSocketError);
using QNetworkReplyErrorSignal = void(QNetworkReply::*)(QNetworkReply::NetworkError);
using QTcpSocketErrorSignal = void(QTcpSocket::*)(QAbstractSocket::SocketError);

inline constexpr auto QLocalSocket_error = QLocalSocketErrorSignal(&QLocalSocket::errorOccurred);
inline constexpr auto QNetworkReply_error = QNetworkReplyErrorSignal(&QNetworkReply::errorOccurred);
inline constexpr auto QTcpSocket_error = QTcpSocketErrorSignal(&QAbstractSocket::errorOccurred);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
using NativeEventResult = qintptr;
using TouchDevice = QInputDevice::DeviceType;
#else // Qt >= 6.0.0
using NativeEventResult = long;
using TouchDevice = QTouchDevice;
#endif // Qt < 6.0.0

[[nodiscard]] inline QStringView StringViewMid(QStringView view, qsizetype pos, qsizetype n = -1) {
	const auto result = ContainerImplHelper::mid(view.size(), &pos, &n);
	return result == ContainerImplHelper::Null ? QStringView() : view.mid(pos, n);
}

} // namespace base
