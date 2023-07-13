// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_info_linux.h"

#include "base/algorithm.h"

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
#include "base/platform/linux/base_linux_xcb_utilities.h"
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

#include <QtCore/QJsonObject>
#include <QtCore/QLocale>
#include <QtCore/QVersionNumber>
#include <QtCore/QDate>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtGui/QGuiApplication>

#include <sys/utsname.h>

#ifdef __GLIBC__
#include <gnu/libc-version.h>
#endif // __GLIBC__

namespace Platform {
namespace {

constexpr auto kMaxDeviceModelLength = 15;

[[nodiscard]] QString GetDesktopEnvironment() {
	const auto value = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
	return value.contains(':')
		? value.left(value.indexOf(':'))
		: value;
}

[[nodiscard]] QString ChassisTypeToString(uint type) {
	switch (type) {
	case 0x3: /* Desktop */
	case 0x4: /* Low Profile Desktop */
	case 0x6: /* Mini Tower */
	case 0x7: /* Tower */
	case 0xD: /* All in one (i.e. PC built into monitor) */
		return "Desktop";
	case 0x8: /* Portable */
	case 0x9: /* Laptop */
	case 0xA: /* Notebook */
	case 0xE: /* Sub Notebook */
		return "Laptop";
	case 0xB: /* Hand Held */
		return "Handset";
	case 0x11: /* Main Server Chassis */
	case 0x1C: /* Blade */
	case 0x1D: /* Blade Enclosure */
		return "Server";
	case 0x1E: /* Tablet */
		return "Tablet";
	case 0x1F: /* Convertible */
	case 0x20: /* Detachable */
		return "Convertible";
	default:
		return "";
	}
}

} // namespace

QString DeviceModelPretty() {
	using namespace base::Platform;
	static const auto result = FinalizeDeviceModel([&] {
		const auto value = [](const char *key) {
			auto file = QFile(u"/sys/class/dmi/id/"_q + key);
			return (file.open(QIODevice::ReadOnly | QIODevice::Text))
				? SimplifyDeviceModel(QString(file.readAll()))
				: QString();
		};
		const auto productName = value("product_name");
		if (const auto model = ProductNameToDeviceModel(productName)
			; !model.isEmpty()) {
			return model;
		}

		const auto productFamily = value("product_family");
		const auto boardName = value("board_name");
		const auto familyName = SimplifyDeviceModel(
			productFamily + ' ' + boardName);

		if (IsDeviceModelOk(familyName)) {
			return familyName;
		} else if (IsDeviceModelOk(boardName)) {
			return boardName;
		} else if (IsDeviceModelOk(productFamily)) {
			return productFamily;
		}

		const auto virtualization = []() -> QString {
			QProcess process;
			process.start("systemd-detect-virt", QStringList());
			process.waitForFinished();
			return process.readAll().simplified().toUpper();
		}();

		if (!virtualization.isEmpty() && virtualization != qstr("NONE")) {
			return virtualization;
		}

		const auto chassisType = ChassisTypeToString(
			value("chassis_type").toUInt());
		if (!chassisType.isEmpty()) {
			return chassisType;
		}

		return QString();
	}());

	return result;
}

QString SystemVersionPretty() {
	static const auto result = [&] {
		QStringList resultList{};

		struct utsname u;
		if (uname(&u) == 0) {
			resultList << u.sysname;
#ifndef Q_OS_LINUX
			resultList << u.release;
#endif // !Q_OS_LINUX
		} else {
			resultList << "Unknown";
		}

		if (const auto desktopEnvironment = GetDesktopEnvironment();
			!desktopEnvironment.isEmpty()) {
			resultList << desktopEnvironment;
		} else if (const auto windowManager = GetWindowManager();
			!windowManager.isEmpty()) {
			resultList << windowManager;
		}

		if (IsWayland()) {
			resultList << "Wayland";
		} else if (IsX11()) {
			if (qEnvironmentVariableIsSet("WAYLAND_DISPLAY")) {
				resultList << "XWayland";
			} else {
				resultList << "X11";
			}
		}

		const auto libcName = GetLibcName();
		const auto libcVersion = GetLibcVersion();
		if (!libcVersion.isEmpty()) {
			if (!libcName.isEmpty()) {
				resultList << libcName;
			} else {
				resultList << "libc";
			}
			resultList << libcVersion;
		}

		return resultList.join(' ');
	}();

	return result;
}

QString SystemCountry() {
	return QLocale::system().name().split('_').last();
}

QString SystemLanguage() {
	const auto system = QLocale::system();
	const auto languages = system.uiLanguages();
	return languages.isEmpty()
		? system.name().split('_').first()
		: languages.front();
}

QDate WhenSystemBecomesOutdated() {
	const auto libcName = GetLibcName();
	const auto libcVersion = GetLibcVersion();

	if (libcName == qstr("glibc") && !libcVersion.isEmpty()) {
		if (QVersionNumber::fromString(libcVersion) < QVersionNumber(2, 28)) {
			return QDate(2023, 7, 1); // Older than CentOS 8.
		}
	}
	return QDate();
}

int AutoUpdateVersion() {
	return 2;
}

QString AutoUpdateKey() {
	return "linux";
}

QString GetLibcName() {
#ifdef __GLIBC__
	return "glibc";
#endif // __GLIBC__

	return QString();
}

QString GetLibcVersion() {
#ifdef __GLIBC__
	static const auto result = [&] {
		const auto version = QString::fromLatin1(gnu_get_libc_version());
		return QVersionNumber::fromString(version).isNull() ? QString() : version;
	}();
	return result;
#endif // __GLIBC__

	return QString();
}

QString GetWindowManager() {
#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
	base::Platform::XCB::CustomConnection connection;
	if (xcb_connection_has_error(connection)) {
		return {};
	}

	const auto root = base::Platform::XCB::GetRootWindow(connection);
	if (!root.has_value()) {
		return {};
	}

	const auto nameAtom = base::Platform::XCB::GetAtom(
		connection,
		"_NET_WM_NAME");

	const auto utf8Atom = base::Platform::XCB::GetAtom(
		connection,
		"UTF8_STRING");

	const auto supportingWindow = base::Platform::XCB::GetSupportingWMCheck(
		connection,
		*root);

	if (!nameAtom.has_value()
		|| !utf8Atom.has_value()
		|| !supportingWindow.has_value()
		|| *supportingWindow == XCB_WINDOW_NONE) {
		return {};
	}

	const auto cookie = xcb_get_property(
		connection,
		false,
		*supportingWindow,
		*nameAtom,
		*utf8Atom,
		0,
		1024);

	const auto reply = base::Platform::XCB::MakeReplyPointer(
		xcb_get_property_reply(
			connection,
			cookie,
			nullptr));

	if (!reply) {
		return {};
	}

	return (reply->format == 8 && reply->type == *utf8Atom)
		? QString::fromUtf8(
			reinterpret_cast<const char*>(
				xcb_get_property_value(reply.get())),
			xcb_get_property_value_length(reply.get()))
		: QString();
#else // !DESKTOP_APP_DISABLE_X11_INTEGRATION
	return QString();
#endif // DESKTOP_APP_DISABLE_X11_INTEGRATION
}

bool IsX11() {
	if (!QGuiApplication::instance()) {
		return qEnvironmentVariableIsSet("DISPLAY");
	}
	static const auto result = (QGuiApplication::platformName() == "xcb");
	return result;
}

bool IsWayland() {
	if (!QGuiApplication::instance()) {
		return qEnvironmentVariableIsSet("WAYLAND_DISPLAY");
	}
	static const auto result
		= QGuiApplication::platformName().startsWith("wayland");
	return result;
}

void Start(QJsonObject options) {
}

void Finish() {
}

} // namespace Platform
