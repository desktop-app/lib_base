// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

namespace base {

[[nodiscard]] QString FileNameFromUserString(QString name);

void RegisterResourceArchive(const QString &name);

} // namespace base
