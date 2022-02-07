// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/mac/base_custom_app_icon_mac.h"

#include "base/debug_log.h"
#include "base/platform/mac/base_utilities_mac.h"

#include <QtGui/QImage>
#include <QtCore/QFile>
#include <QtCore/QTemporaryFile>

#include <sys/xattr.h>

namespace base::Platform {
namespace {

using namespace ::Platform;

constexpr auto kFinderInfo = "com.apple.FinderInfo";

// We want to write [8], so just in case.
constexpr auto kFinderInfoMinSize = 16;

// Usually.
constexpr auto kFinderInfoSize = 32;

// Just in case.
constexpr auto kFinderInfoMaxSize = 256;

[[nodiscard]] QString BundlePath() {
	@autoreleasepool {

	NSString *path = @"";
	@try {
		path = [[NSBundle mainBundle] bundlePath];
		if (!path) {
			Unexpected("Could not get bundled path!");
		}
		return QFile::decodeName([path fileSystemRepresentation]);
	}
	@catch (NSException *exception) {
		Unexpected("Exception in resource registering.");
	}

	}
}

int Launch(const QString &command, const QStringList &arguments) {
	@autoreleasepool {

	@try {

	NSMutableArray *list = [NSMutableArray arrayWithCapacity:arguments.size()];
	for (const auto &argument : arguments) {
		[list addObject:Q2NSString(argument)];
	}

	NSTask *task = [[NSTask alloc] init];
	task.launchPath = Q2NSString(command);
	task.arguments = list;

	[task launch];
	[task waitUntilExit];

	return [task terminationStatus];

	}
	@catch (NSException *exception) {
		return -888;
	}
	@finally {
	}

	}
}

[[nodiscard]] std::optional<std::string> ReadCustomIconAttribute(const QString &bundle) {
	const auto native = QFile::encodeName(bundle).toStdString();
	auto info = std::array<char, kFinderInfoMaxSize>();
	const auto result = getxattr(
		native.c_str(),
		kFinderInfo,
		info.data(),
		info.size(),
		0, // position
		XATTR_NOFOLLOW);
	const auto error = (result < 0) ? errno : 0;
	if (result < 0) {
		if (error == ENOATTR) {
			return std::string();
		} else {
			LOG(("Icon Error: Could not get %1 xattr, error: %2."
				).arg(kFinderInfo
				).arg(error));
			return std::nullopt;
		}
	} else if (result < kFinderInfoMinSize) {
		LOG(("Icon Error: Bad existing %1 xattr size: %2."
			).arg(kFinderInfo
			).arg(error));
		return std::nullopt;
	}
	return std::string(info.data(), result);
}

[[nodiscard]] bool WriteCustomIconAttribute(
		const QString &bundle,
		const std::string &value) {
	const auto native = QFile::encodeName(bundle).toStdString();
	const auto result = setxattr(
		native.c_str(),
		kFinderInfo,
		value.data(),
		value.size(),
		0, // position
		XATTR_NOFOLLOW);
	if (result != 0) {
		LOG(("Icon Error: Could not set %1 xattr, error: %2."
			).arg(kFinderInfo
			).arg(errno));
		return false;
	}
	return true;
}

[[nodiscard]] bool DeleteCustomIconAttribute(const QString &bundle) {
	const auto native = QFile::encodeName(bundle).toStdString();
	const auto result = removexattr(
		native.c_str(),
		kFinderInfo,
		XATTR_NOFOLLOW);
	if (result != 0) {
		LOG(("Icon Error: Could not remove %1 xattr, error: %2."
			).arg(kFinderInfo
			).arg(errno));
		return false;
	}
	return true;
}

[[nodiscard]] bool EnableCustomIcon(const QString &bundle) {
	auto info = ReadCustomIconAttribute(bundle);
	if (!info.has_value()) {
		return false;
	} else if (info->empty()) {
		*info = std::string(kFinderInfoSize, char(0));
	}
	if ((*info)[8] & 0x04) {
		(*info)[8] &= ~0x04;
		if (!WriteCustomIconAttribute(bundle, *info)) {
			return false;
		}
	}
	(*info)[8] |= 4;
	return WriteCustomIconAttribute(bundle, *info);
}

[[nodiscard]] bool DisableCustomIcon(const QString &bundle) {
	auto info = ReadCustomIconAttribute(bundle);
	if (!info.has_value()) {
		return false;
	} else if (info->empty()) {
		return true;
	}
	return DeleteCustomIconAttribute(bundle);
}

[[nodiscard]] bool RefreshDock() {
	Launch("/bin/bash", { "-c", "rm /var/folders/*/*/*/com.apple.dock.iconcache" });

	const auto killall = Launch("/usr/bin/killall", { "Dock" });
	if (killall != 0) {
		LOG(("Icon Error: Failed to run `killall Dock`, result: %1.").arg(killall));
		return false;
	}
	return true;
}

} // namespace

bool SetCustomAppIcon(const QString &path) {
	const auto icns = path.endsWith(".icns", Qt::CaseInsensitive);
	const auto temp = [&] {
		const auto extension = QString(icns ? "icns" : "png");
		auto file = QTemporaryFile("custom_icon_XXXXXX." + extension);
		file.setAutoRemove(false);
		return file.open() ? file.fileName() : QString();
	}();
	if (temp.isEmpty()) {
		LOG(("Icon Error: Could not obtain a temporary file name."));
		return false;
	}
	const auto guard = gsl::finally([&] { QFile::remove(temp); });
	if (icns) {
		QFile::remove(temp);
		if (!QFile(path).copy(temp)) {
			LOG(("Icon Error: Failed to copy icon from \"%1\" to \"%2\"."
				).arg(path
				).arg(temp));
			return false;
		}
	} else {
		auto image = QImage(path);
		if (image.isNull()) {
			LOG(("Icon Error: Failed to read image from \"%1\".").arg(path));
			return false;
		}
		image = std::move(image).convertToFormat(QImage::Format_ARGB32);
		if (image.isNull()) {
			LOG(("Icon Error: Failed to convert image to ARGB32."));
			return false;
		}
		if (!image.save(temp, "PNG")) {
			LOG(("Icon Error: Failed to save image to \"%1\".").arg(temp));
			return false;
		}
	}
	const auto sips = Launch("/usr/bin/sips", {
		"-i",
		temp
	});
	if (sips != 0) {
		LOG(("Icon Error: Failed to run `sips -i \"%1\"`, result: %2.").arg(temp).arg(sips));
		return false;
	}
	const auto bundle = BundlePath();
	const auto icon = bundle + "/Icon\r";
	const auto touch = Launch("/usr/bin/touch", { icon });
	if (touch != 0) {
		LOG(("Icon Error: Failed to run `touch \"%1\"`, result: %2.").arg(icon).arg(sips));
		return false;
	}
	const auto from = temp + "/..namedfork/rsrc";
	const auto to = icon + "/..namedfork/rsrc";
	const auto cp = Launch("/bin/cp", { from, to });
	if (cp != 0) {
		LOG(("Icon Error: Failed to run `cp \"%1\" \"%2\"`, result: %3."
			).arg(from
			).arg(to
			).arg(cp));
		return false;
	} else if (!EnableCustomIcon(bundle)) {
		return false;
	}
	return RefreshDock();
}

bool ClearCustomAppIcon() {
	const auto bundle = BundlePath();
	const auto icon = bundle + "/Icon\r";
	Launch("/bin/rm", { icon });
	auto info = ReadCustomIconAttribute(bundle);
	if (!info.has_value()) {
		return false;
	} else if (info->empty()) {
		return true;
	} else if (!DeleteCustomIconAttribute(bundle)) {
		return false;
	}
	return RefreshDock();
}

} // namespace base::Platform
