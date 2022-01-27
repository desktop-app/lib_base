// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

namespace Platform::ConfirmQuit {

// A "{key}" substring will be replaced by the key combination.
[[nodiscard]] bool RunModal(QString text);

} // namespace Platform::ConfirmQuit
