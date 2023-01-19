// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#ifdef _DEBUG

#pragma once

namespace base {

void DebugHotkeyPresses(Fn<void()> callback);
rpl::producer<> DebugHotkeyPresses();

} // namespace base

#endif // !_DEBUG
