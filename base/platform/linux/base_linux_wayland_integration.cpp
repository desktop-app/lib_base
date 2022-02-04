/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "base/platform/linux/base_linux_wayland_integration.h"

#include "base/platform/base_platform_info.h"
#include "base/flat_map.h"

#include <QtCore/QPointer>
#include <QtGui/QWindow>

#include <connection_thread.h>
#include <registry.h>
#include <surface.h>
#include <xdgforeign.h>
#include <idleinhibit.h>

using namespace KWayland::Client;

namespace base {
namespace Platform {

struct WaylandIntegration::Private {
	std::unique_ptr<ConnectionThread> connection;
	Registry registry;
	std::unique_ptr<XdgExporter> xdgExporter;
	std::unique_ptr<IdleInhibitManager> idleInhibitManager;
	base::flat_map<QWindow*, QPointer<IdleInhibitor>> idleInhibitorMap;
};

WaylandIntegration::WaylandIntegration()
: _private(std::make_unique<Private>()) {
	_private->connection = std::unique_ptr<ConnectionThread>{
		ConnectionThread::fromApplication(),
	};

	_private->registry.create(_private->connection.get());
	_private->registry.setup();

	QObject::connect(
		_private->connection.get(),
		&ConnectionThread::connectionDied,
		&_private->registry,
		&Registry::destroy);

	QObject::connect(
		&_private->registry,
		&Registry::exporterUnstableV2Announced,
		[=](uint name, uint version) {
			_private->xdgExporter = std::unique_ptr<XdgExporter>{
				_private->registry.createXdgExporter(name, version),
			};

			QObject::connect(
				_private->connection.get(),
				&ConnectionThread::connectionDied,
				_private->xdgExporter.get(),
				&XdgExporter::destroy);
		});

	QObject::connect(
		&_private->registry,
		&Registry::idleInhibitManagerUnstableV1Announced,
		[=](uint name, uint version) {
			_private->idleInhibitManager = std::unique_ptr<IdleInhibitManager>{
				_private->registry.createIdleInhibitManager(name, version),
			};

			QObject::connect(
				_private->connection.get(),
				&ConnectionThread::connectionDied,
				_private->idleInhibitManager.get(),
				&IdleInhibitManager::destroy);
		});
}

WaylandIntegration::~WaylandIntegration() = default;

WaylandIntegration *WaylandIntegration::Instance() {
	if (!::Platform::IsWayland()) return nullptr;
	static WaylandIntegration instance;
	return &instance;
}

QString WaylandIntegration::nativeHandle(QWindow *window) {
	if (const auto exporter = _private->xdgExporter.get()) {
		if (const auto surface = Surface::fromWindow(window)) {
			if (const auto exported = exporter->exportTopLevel(
				surface,
				surface)) {
				QEventLoop loop;
				QObject::connect(
					exported,
					&XdgExported::done,
					&loop,
					&QEventLoop::quit);
				loop.exec();
				return exported->handle();
			}
		}
	}
	return {};
}

void WaylandIntegration::preventDisplaySleep(bool prevent, QWindow *window) {
	if (!prevent) {
		auto it = _private->idleInhibitorMap.find(window);
		if (it != _private->idleInhibitorMap.cend()) {
			if (it->second) {
				delete it->second;
			}
			_private->idleInhibitorMap.erase(it);
		}
		return;
	}

	const auto manager = _private->idleInhibitManager.get();
	if (!manager) {
		return;
	}

	const auto surface = Surface::fromWindow(window);
	if (!surface) {
		return;
	}

	_private->idleInhibitorMap.emplace(window, manager->createInhibitor(surface, surface));
}

} // namespace Platform
} // namespace base
