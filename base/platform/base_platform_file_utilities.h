// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

class QString;
class QFile;

namespace base::Platform {

void ShowInFolder(const QString &filepath);
[[nodiscard]] QString FileNameFromUserString(QString name);

bool DeleteDirectory(QString path);
void RemoveQuarantine(const QString &path);

[[nodiscard]] QString CurrentExecutablePath(int argc, char *argv[]);
[[nodiscard]] QString BundledResourcesPath();

bool RenameWithOverwrite(const QString &from, const QString &to);
void FlushFileData(QFile &file);

} // namespace base::Platform
