// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_linux_gtk_integration.h"

#include "base/platform/linux/base_linux_gtk_integration_p.h"
#include "base/platform/base_platform_info.h"
#include "base/integration.h"

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
#include "base/platform/linux/base_linux_xlib_helper.h"
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

#include <QtGui/QIcon>

namespace base {
namespace Platform {

using namespace Gtk;

namespace {

bool TriedToInit = false;
bool Loaded = false;

bool LoadLibrary(QLibrary &lib, const char *name, int version) {
#ifdef LINK_TO_GTK
	return true;
#else // LINK_TO_GTK
	Integration::Instance().logMessage(
		QString("Loading '%1' with version %2...")
			.arg(QLatin1String(name))
			.arg(version));
	lib.setFileNameAndVersion(QLatin1String(name), version);
	if (lib.load()) {
		Integration::Instance().logMessage(
			QString("Loaded '%1' with version %2!")
				.arg(QLatin1String(name))
				.arg(version));
		return true;
	}
	lib.setFileNameAndVersion(QLatin1String(name), QString());
	if (lib.load()) {
		Integration::Instance().logMessage(
			QString("Loaded '%1' without version!")
				.arg(QLatin1String(name)));
		return true;
	}
	Integration::Instance().logMessage(
		QString("Could not load '%1' with version %2 :(")
			.arg(QLatin1String(name))
			.arg(version));
	return false;
#endif // !LINK_TO_GTK
}

void GtkMessageHandler(
		const gchar *log_domain,
		GLogLevelFlags log_level,
		const gchar *message,
		gpointer unused_data) {
	// Silence false-positive Gtk warnings (we are using Xlib to set
	// the WM_TRANSIENT_FOR hint).
	if (message != qstr("GtkDialog mapped without a transient parent. "
		"This is discouraged.")) {
		// For other messages, call the default handler.
		g_log_default_handler(log_domain, log_level, message, unused_data);
	}
}

bool SetupGtkBase(QLibrary &lib) {
	if (!LOAD_GTK_SYMBOL(lib, "gtk_init_check", gtk_init_check)) return false;

	if (LOAD_GTK_SYMBOL(lib, "gdk_set_allowed_backends", gdk_set_allowed_backends)) {
		// We work only with Wayland and X11 GDK backends.
		// Otherwise we get segfault in Ubuntu 17.04 in gtk_init_check() call.
		// See https://github.com/telegramdesktop/tdesktop/issues/3176
		// See https://github.com/telegramdesktop/tdesktop/issues/3162
		if(::Platform::IsWayland()) {
			Integration::Instance().logMessage(
				"Limit allowed GDK backends to wayland,x11");
			gdk_set_allowed_backends("wayland,x11");
		} else {
			Integration::Instance().logMessage(
				"Limit allowed GDK backends to x11,wayland");
			gdk_set_allowed_backends("x11,wayland");
		}
	}

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
	// gtk_init will reset the Xlib error handler,
	// and that causes Qt applications to quit on X errors.
	// Therefore, we need to manually restore it.
	XErrorHandlerRestorer handlerRestorer;
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

	Integration::Instance().logMessage("Library gtk functions loaded!");
	TriedToInit = true;
	if (!gtk_init_check(0, 0)) {
		gtk_init_check = nullptr;
		Integration::Instance().logMessage(
			"Failed to gtk_init_check(0, 0)!");
		return false;
	}
	Integration::Instance().logMessage("Checked gtk with gtk_init_check!");

	// Use our custom log handler.
	g_log_set_handler("Gtk", G_LOG_LEVEL_MESSAGE, GtkMessageHandler, nullptr);

	return true;
}

template <typename T>
std::optional<T> GtkSetting(const QString &propertyName) {
	if (gtk_settings_get_default == nullptr) {
		return std::nullopt;
	}
	T value;
	g_object_get(
		gtk_settings_get_default(),
		propertyName.toUtf8().constData(),
		&value,
		nullptr);
	return value;
}

bool IconThemeShouldBeSet() {
	// change the icon theme only if
	// it isn't already set by a platformtheme plugin
	static const auto Result =
		// QGenericUnixTheme
		(QIcon::themeName() == qstr("hicolor")
			&& QIcon::fallbackThemeName() == qstr("hicolor"))
		// QGnomeTheme
		|| (QIcon::themeName() == qstr("Adwaita")
			&& QIcon::fallbackThemeName() == qstr("gnome"))
		// qt5ct
		|| (QIcon::themeName().isEmpty()
			&& QIcon::fallbackThemeName().isEmpty());

	return Result;
}

bool CursorSizeShouldBeSet() {
	// change the cursor size only on Wayland and if it wasn't already set
	static const auto Result = ::Platform::IsWayland()
		&& qEnvironmentVariableIsEmpty("XCURSOR_SIZE");

	return Result;
}

void SetIconTheme() {
	static const auto setter = [] {
		const auto integration = GtkIntegration::Instance();
		if (!integration || !IconThemeShouldBeSet()) {
			return;
		}

		const auto themeName = integration->getStringSetting(
			"gtk-icon-theme-name");

		const auto fallbackThemeName = integration->getStringSetting(
			"gtk-fallback-icon-theme");

		if (!themeName.has_value() || !fallbackThemeName.has_value()) {
			return;
		}

		Integration::Instance().logMessage("Setting GTK icon theme");

		QIcon::setThemeName(*themeName);
		QIcon::setFallbackThemeName(*fallbackThemeName);

		Integration::Instance().logMessage(
			QString("New icon theme: %1")
				.arg(QIcon::themeName()));

		Integration::Instance().logMessage(
			QString("New fallback icon theme: %1")
				.arg(QIcon::fallbackThemeName()));
	};

	if (QCoreApplication::instance()) {
		Integration::Instance().enterFromEventLoop(setter);
	} else {
		setter();
	}
}

void SetCursorSize() {
	static const auto setter = [] {
		const auto integration = GtkIntegration::Instance();
		if (!integration || !CursorSizeShouldBeSet()) {
			return;
		}

		const auto newCursorSize = integration->getIntSetting(
			"gtk-cursor-theme-size");

		if (!newCursorSize.has_value()) {
			return;
		}

		Integration::Instance().logMessage("Setting GTK cursor size");
		qputenv("XCURSOR_SIZE", QByteArray::number(*newCursorSize));
		Integration::Instance().logMessage(
			QString("New cursor size: %1").arg(*newCursorSize));
	};

	if (QCoreApplication::instance()) {
		Integration::Instance().enterFromEventLoop(setter);
	} else {
		setter();
	}
}

} // namespace

GtkIntegration::GtkIntegration() {
}

GtkIntegration *GtkIntegration::Instance() {
	static const auto useGtkIntegration = !qEnvironmentVariableIsSet(
		kDisableGtkIntegration.utf8().constData());
	
	if (!useGtkIntegration) {
		return nullptr;
	}

	static GtkIntegration instance;
	return &instance;
}

void GtkIntegration::prepareEnvironment() {
#ifdef DESKTOP_APP_USE_PACKAGED // static binary doesn't contain qgtk3/qgtk2
	// if gtk integration and qgtk3/qgtk2 platformtheme (or qgtk2 style)
	// is used at the same time, the app will crash
	if (!qEnvironmentVariableIsSet(
			kIgnoreGtkIncompatibility.utf8().constData())) {
		g_warning(
			"Unfortunately, GTK integration "
			"conflicts with qgtk2 platformtheme and style. "
			"Therefore, QT_QPA_PLATFORMTHEME "
			"and QT_STYLE_OVERRIDE will be unset.");

		g_message(
			"This can be ignored by setting %s environment variable "
			"to any value, however, if qgtk2 theme or style is used, "
			"this will lead to a crash.",
			kIgnoreGtkIncompatibility.utf8().constData());

		g_message(
			"GTK integration can be disabled by setting %s to any value. "
			"Keep in mind that this will lead to "
			"some features being unavailable.",
			kDisableGtkIntegration.utf8().constData());

		qunsetenv("QT_QPA_PLATFORMTHEME");
		qunsetenv("QT_STYLE_OVERRIDE");
	}
#endif // DESKTOP_APP_USE_PACKAGED
}

void GtkIntegration::load() {
	Expects(!loaded());
	Integration::Instance().logMessage("Loading GTK");

	Integration::Instance().logMessage(
		QString("Icon theme: %1")
			.arg(QIcon::themeName()));

	Integration::Instance().logMessage(
		QString("Fallback icon theme: %1")
			.arg(QIcon::fallbackThemeName()));

	_lib.setLoadHints(QLibrary::DeepBindHint);

	if (LoadLibrary(_lib, "gtk-3", 0)) {
		Loaded = SetupGtkBase(_lib);
	}
	if (!Loaded
		&& !TriedToInit
		&& LoadLibrary(_lib, "gtk-x11-2.0", 0)) {
		Loaded = SetupGtkBase(_lib);
	}

	if (Loaded) {
		LOAD_GTK_SYMBOL(_lib, "gtk_check_version", gtk_check_version);
		LOAD_GTK_SYMBOL(_lib, "gtk_settings_get_default", gtk_settings_get_default);

		SetIconTheme();
		SetCursorSize();
		connectToSetting("gtk-icon-theme-name", SetIconTheme);
		connectToSetting("gtk-cursor-theme-size", SetCursorSize);
	} else {
		Integration::Instance().logMessage("Could not load gtk-3 or gtk-x11-2.0!");
	}
}

bool GtkIntegration::loaded() const {
	return Loaded;
}

bool GtkIntegration::checkVersion(uint major, uint minor, uint micro) const {
	return (gtk_check_version != nullptr)
		? !gtk_check_version(major, minor, micro)
		: false;
}

std::optional<bool> GtkIntegration::getBoolSetting(
		const QString &propertyName) const {
	const auto value = GtkSetting<gboolean>(propertyName);
	if (!value.has_value()) {
		return std::nullopt;
	}
	Integration::Instance().logMessage(
		QString("Getting GTK setting, %1: %2")
			.arg(propertyName)
			.arg(*value ? "[TRUE]" : "[FALSE]"));
	return *value;
}

std::optional<int> GtkIntegration::getIntSetting(
		const QString &propertyName) const {
	const auto value = GtkSetting<gint>(propertyName);
	if (value.has_value()) {
		Integration::Instance().logMessage(
			QString("Getting GTK setting, %1: %2")
				.arg(propertyName)
				.arg(*value));
	}
	return value;
}

std::optional<QString> GtkIntegration::getStringSetting(
		const QString &propertyName) const {
	auto value = GtkSetting<gchararray>(propertyName);
	if (!value.has_value()) {
		return std::nullopt;
	}
	const auto str = QString::fromUtf8(*value);
	g_free(*value);
	Integration::Instance().logMessage(
		QString("Getting GTK setting, %1: '%2'")
			.arg(propertyName)
			.arg(str));
	return str;
}

void GtkIntegration::connectToSetting(
		const QString &propertyName,
		void (*callback)()) {
	if (gtk_settings_get_default == nullptr) {
		return;
	}

	g_signal_connect(
		gtk_settings_get_default(),
		("notify::" + propertyName).toUtf8().constData(),
		G_CALLBACK(callback),
		nullptr);
}

} // namespace Platform
} // namespace base
