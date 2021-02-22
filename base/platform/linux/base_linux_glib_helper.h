/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

extern "C" {
#undef signals
#include <gio/gio.h>
#define signals public
} // extern "C"

namespace base::Platform {

struct gobject_deleter {
	void operator()(gpointer value) const {
		if (value) {
			g_object_unref(value);
		}
	}
};

template <typename Type>
using gobject_ptr = std::unique_ptr<Type, gobject_deleter>;

template <typename Type>
gobject_ptr<Type> gobject_wrap(Type *value) {
	return gobject_ptr<Type>(value, gobject_deleter());
}

} // namespace base::Platform
