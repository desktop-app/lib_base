/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "base/platform/linux/base_linux_xlib_helper.h"

extern "C" {
#include <X11/Xlib.h>
}

namespace base {
namespace Platform {

class XErrorHandlerRestorer::Private {
public:
	Private()
	: _oldErrorHandler(XSetErrorHandler(nullptr)) {
	}

	~Private() {
		XSetErrorHandler(_oldErrorHandler);
	}

private:
	int (*_oldErrorHandler)(Display *, XErrorEvent *);

};

XErrorHandlerRestorer::XErrorHandlerRestorer()
: _private(std::make_unique<Private>()) {
}

XErrorHandlerRestorer::~XErrorHandlerRestorer() = default;

void InitXThreads() {
	XInitThreads();
}

} // namespace Platform
} // namespace base
