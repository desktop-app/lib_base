// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_info_linux.h"

#include "base/algorithm.h"
#include "base/platform/linux/base_linux_library.h"

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

extern "C" {
struct wl_display;
} // extern "C"

namespace Platform {
namespace {

[[nodiscard]] QStringList GetDesktopEnvironment() {
	return qEnvironmentVariable("XDG_CURRENT_DESKTOP").trimmed().split(':');
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

[[nodiscard]] bool IsGlibcLess228() {
	static const auto result = [] {
		const auto libcName = GetLibcName();
		const auto libcVersion = GetLibcVersion();
		return (libcName == qstr("glibc"))
			&& !libcVersion.isEmpty()
			&& (QVersionNumber::fromString(libcVersion)
				< QVersionNumber(2, 28));
	}();
	return result;
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
		} else if (IsXwayland()) {
			resultList << "Xwayland";
		} else if (IsX11()) {
			resultList << "X11";
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
	if (IsGlibcLess228()) {
		return QDate(2023, 7, 1); // Older than CentOS 8.
	}
	return QDate();
}

int AutoUpdateVersion() {
	if (IsGlibcLess228()) {
		return 2;
	}
	return 4;
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
	const base::Platform::XCB::Connection connection;
	if (!connection || xcb_connection_has_error(connection)) {
		return {};
	}

	const auto root = base::Platform::XCB::GetRootWindow(connection);
	if (!root) {
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
		root);

	if (!nameAtom || !utf8Atom || !supportingWindow) {
		return {};
	}

	const auto cookie = xcb_get_property(
		connection,
		false,
		supportingWindow,
		nameAtom,
		utf8Atom,
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

	return (reply->format == 8 && reply->type == utf8Atom)
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
		static const auto result = []() -> bool {
#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
			const base::Platform::XCB::Connection connection;
			return connection && !xcb_connection_has_error(connection);
#else // !DESKTOP_APP_DISABLE_X11_INTEGRATION
			return qEnvironmentVariableIsSet("DISPLAY");
#endif // DESKTOP_APP_DISABLE_X11_INTEGRATION
		}();
		return result;
	}
	static const auto result = (QGuiApplication::platformName() == "xcb");
	return result;
}

bool IsWayland() {
	if (!QGuiApplication::instance()) {
		static const auto result = []() -> bool {
			struct wl_display *(*wl_display_connect)(const char *name);
			void (*wl_display_disconnect)(struct wl_display *display);
			if (const auto lib = base::Platform::LoadLibrary(
					"libwayland-client.so.0",
					RTLD_NODELETE); lib
					&& LOAD_LIBRARY_SYMBOL(lib, wl_display_connect)
					&& LOAD_LIBRARY_SYMBOL(lib, wl_display_disconnect)) {
				const auto display = wl_display_connect(nullptr);
				if (display) {
					wl_display_disconnect(display);
				}
				return display;
			}
			return qEnvironmentVariableIsSet("WAYLAND_DISPLAY");
		}();
		return result;
	}
	static const auto result
		= QGuiApplication::platformName().startsWith("wayland");
	return result;
}

bool IsXwayland() {
#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
	static const auto result = []() -> bool {
		const base::Platform::XCB::Connection connection;
		if (!connection || xcb_connection_has_error(connection)) {
			return false;
		}
		constexpr auto kXWAYLAND = "XWAYLAND";
		const auto reply = base::Platform::XCB::MakeReplyPointer(
			xcb_query_extension_reply(
				connection,
				xcb_query_extension(
					connection,
					strlen(kXWAYLAND),
					kXWAYLAND),
				nullptr));
		if (!reply) {
			return false;
		}
		return reply->present;
	}();
	return result;
#else // !DESKTOP_APP_DISABLE_X11_INTEGRATION
	return IsX11() && qEnvironmentVariableIsSet("WAYLAND_DISPLAY");
#endif // DESKTOP_APP_DISABLE_X11_INTEGRATION
}

void Start(QJsonObject options) {
}

void Finish() {
}

} // namespace Platform
