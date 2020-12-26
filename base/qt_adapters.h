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

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#include <QtGui/QColorSpace>
#endif // Qt >= 5.14

namespace base {

using QLocalSocketErrorSignal = void(QLocalSocket::*)(QLocalSocket::LocalSocketError);
using QNetworkReplyErrorSignal = void(QNetworkReply::*)(QNetworkReply::NetworkError);
using QTcpSocketErrorSignal = void(QTcpSocket::*)(QAbstractSocket::SocketError);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
inline constexpr auto QLocalSocket_error = QLocalSocketErrorSignal(&QLocalSocket::errorOccurred);
inline constexpr auto QNetworkReply_error = QNetworkReplyErrorSignal(&QNetworkReply::errorOccurred);
inline constexpr auto QTcpSocket_error = QTcpSocketErrorSignal(&QAbstractSocket::errorOccurred);
#else // Qt >= 5.15
inline constexpr auto QLocalSocket_error = QLocalSocketErrorSignal(&QLocalSocket::error);
inline constexpr auto QNetworkReply_error = QNetworkReplyErrorSignal(&QNetworkReply::error);
inline constexpr auto QTcpSocket_error = QTcpSocketErrorSignal(&QAbstractSocket::error);
#endif // Qt >= 5.15

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
inline constexpr auto QStringSkipEmptyParts = Qt::SkipEmptyParts;
#else // Qt >= 5.14
inline constexpr auto QStringSkipEmptyParts = QString::SkipEmptyParts;
#endif // Qt >= 5.14

[[nodiscard]] inline QScreen *QScreenNearestTo(const QPoint &point) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
	const auto result = QGuiApplication::screenAt(point);
#else // Qt >= 5.10
	const auto result = [&]() -> QScreen* {
		const auto list = QGuiApplication::screens();
		for (const auto screen : list) {
			if (screen->geometry().contains(point)) {
				return screen;
			}
		}
		const auto proj = [&](QScreen *screen) {
			return (screen->geometry().center() - point).manhattanLength();
		};
		const auto i = ranges::min_element(list, ranges::less(), proj);
		return (i == list.end()) ? nullptr : *i;
	}();
#endif // Qt >= 5.10
	return result ? result : QGuiApplication::primaryScreen();
}

[[nodiscard]] inline QDateTime QDateToDateTime(const QDate &date) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	return date.startOfDay();
#else // Qt >= 5.14
	return QDateTime(date);
#endif // Qt >= 5.14
}

inline void QClearColorSpace(QImage &image) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	image.setColorSpace(QColorSpace());
#endif // Qt >= 5.14
}

} // namespace base
