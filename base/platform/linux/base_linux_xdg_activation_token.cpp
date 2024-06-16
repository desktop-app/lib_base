// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_linux_xdg_activation_token.h"

#include "base/invoke_queued.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <qpa/qplatformwindow_p.h>

namespace base::Platform {

void RunWithXdgActivationToken(Fn<void(QString)> callback) {
#if defined QT_FEATURE_wayland && QT_CONFIG(wayland)
	const auto window = QGuiApplication::focusWindow();
	if (!window) {
		callback({});
		return;
	} else if (!window->isVisible()) {
		InvokeQueued(qApp, [=] {
			RunWithXdgActivationToken(callback);
		});
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
#else // wayland
	callback({});
#endif // !wayland
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
