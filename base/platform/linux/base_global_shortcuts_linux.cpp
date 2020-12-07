// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_global_shortcuts_linux.h"

namespace base::Platform::GlobalShortcuts {
namespace {

Fn<void(GlobalShortcutKeyGeneric descriptor, bool down)> ProcessCallback;

} // namespace

bool Available() {
	return false;
}

bool Allowed() {
	return false;
}

void Start(Fn<void(GlobalShortcutKeyGeneric descriptor, bool down)> process) {
	ProcessCallback = std::move(process);
}

void Stop() {
}

QString KeyName(GlobalShortcutKeyGeneric descriptor) {
	return QString("\\x%1").arg(descriptor, 0, 16);
}

} // namespace base::Platform::GlobalShortcuts
