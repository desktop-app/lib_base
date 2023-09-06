/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "base/platform/linux/base_linux_xdg_activation_token.h"

#include "base/invoke_queued.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <qpa/qplatformwindow_p.h>

namespace base::Platform {

void RunWithXdgActivationToken(Fn<void(QString)> callback) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
	const auto window = QGuiApplication::focusWindow();
	if (!window) {
		callback({});
		return;
	}

	using namespace QNativeInterface;
	using namespace QNativeInterface::Private;
	const auto native = qApp->nativeInterface<QWaylandApplication>();
	const auto nativeWindow = window->nativeInterface<QWaylandWindow>();
	if (!native || !nativeWindow) {
		callback({});
		return;
	}

	QObject::connect(
		nativeWindow,
		&QWaylandWindow::xdgActivationTokenCreated,
		nativeWindow,
		callback,
		Qt::SingleShotConnection);

	nativeWindow->requestXdgActivationToken(native->lastInputSerial());
#else // Qt >= 6.5.0
	callback({});
#endif // Qt < 6.5.0
}

QString XdgActivationToken() {
	QString result;
	QEventLoop loop;
	InvokeQueued(qApp, [&] {
		RunWithXdgActivationToken([&](QString token) {
			result = token;
			loop.quit();
		});
	});
	loop.exec();
	return result;
}

} // namespace base::Platform
