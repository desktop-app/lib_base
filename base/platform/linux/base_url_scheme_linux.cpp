// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_url_scheme_linux.h"

#include "base/integration.h"
#include "base/debug_log.h"

#include <QtGui/QGuiApplication>
#include <QtWidgets/QWidget>

#include <kshell.h>
#include <ksandbox.h>

#include <gio/gio.hpp>
#include <snapcraft/snapcraft.hpp>

namespace base::Platform {
namespace {

using namespace gi::repository;

void SnapDefaultHandler(const QString &protocol) {
    Snapcraft::SettingsProxy::new_for_bus(
        Gio::BusType::SESSION_,
        Gio::DBusProxyFlags::NONE_,
        "io.snapcraft.Settings",
        "/io/snapcraft/Settings",
        [=](GObject::Object, Gio::AsyncResult res) {
            auto interface = Snapcraft::Settings(
                Snapcraft::SettingsProxy::new_for_bus_finish(res, nullptr));

            if (!interface) {
                return;
            }

            interface.call_get_sub(
                "default-url-scheme-handler",
                protocol.toStdString(),
                [=](GObject::Object, Gio::AsyncResult res) mutable {
                    const auto currentHandler = [&]()
                    -> std::optional<std::string> {
                        if (auto result = interface.call_get_sub_finish(
                                res)) {
                            return std::get<1>(*result).opt_();
                        }
                        return std::nullopt;
                    }();

                    if (!currentHandler) {
                        return;
                    }

                    const auto &integration = Integration::Instance();
                    const auto expectedHandler = integration.executableName()
                        + u".desktop"_q;

                    if (currentHandler->c_str() == expectedHandler) {
                        return;
                    }

                    const auto window = std::make_shared<QWidget>();
                    window->setAttribute(Qt::WA_DontShowOnScreen);
                    window->setWindowModality(Qt::ApplicationModal);
                    window->show();

                    interface.call_set_sub(
                        "default-url-scheme-handler",
                        protocol.toStdString(),
                        expectedHandler.toStdString(),
                        [=](GObject::Object, Gio::AsyncResult) {
                            (void)window; // don't destroy until finish
                        });
                });
        });
}

} // namespace

bool CheckUrlScheme(const UrlSchemeDescriptor &descriptor) {
    const auto handlerType = "x-scheme-handler/"
        + descriptor.protocol.toStdString();

    const auto neededCommandline = KShell::joinArgs(QStringList{
        descriptor.executable,
    } + KShell::splitArgs(descriptor.arguments) + QStringList{
        "--",
        "%u",
    }).toStdString();

    auto currentAppInfo = Gio::AppInfo::get_default_for_type(
        handlerType,
        true);

    if (currentAppInfo) {
        return currentAppInfo.get_commandline() == neededCommandline;
    }

    return false;
}

void RegisterUrlScheme(const UrlSchemeDescriptor &descriptor) {
    if (KSandbox::isSnap()) {
        SnapDefaultHandler(descriptor.protocol);
        return;
    }

    if (CheckUrlScheme(descriptor)) {
        return;
    }
    UnregisterUrlScheme(descriptor);

    const auto handlerType = "x-scheme-handler/"
        + descriptor.protocol.toStdString();

    const auto commandlineForCreator = KShell::joinArgs(QStringList{
        descriptor.executable,
    } + KShell::splitArgs(descriptor.arguments) + QStringList{
        "--",
    }).toStdString();

    const auto appId = QGuiApplication::desktopFileName().toStdString();
    if (!appId.empty()) {
        Gio::AppInfo appInfo = Gio::DesktopAppInfo::new_(appId + ".desktop");
        if (appInfo) {
            if (appInfo.get_commandline() == commandlineForCreator + " %u") {
                appInfo.set_as_default_for_type(handlerType);
                return;
            }
        }
    }

    auto newAppInfo = Gio::AppInfo::create_from_commandline(
        commandlineForCreator,
        descriptor.displayAppName.toStdString(),
        Gio::AppInfoCreateFlags::SUPPORTS_URIS_,
        nullptr);

    if (newAppInfo) {
        newAppInfo.set_as_default_for_type(handlerType);
    }
}

void UnregisterUrlScheme(const UrlSchemeDescriptor &descriptor) {
    const auto handlerType = "x-scheme-handler/"
        + descriptor.protocol.toStdString();

    const auto neededCommandline = KShell::joinArgs(QStringList{
        descriptor.executable,
    } + KShell::splitArgs(descriptor.arguments) + QStringList{
        "--",
        "%u",
    }).toStdString();

    auto registeredAppInfos = Gio::AppInfo::get_recommended_for_type(
        handlerType);

    for (auto &appInfo : registeredAppInfos) {
        if (appInfo.get_commandline() == neededCommandline
            && !std::string(appInfo.get_id()).compare(0, 8, "userapp-")) {
            appInfo.delete_();
        }
    }
}

} // namespace base::Platform
