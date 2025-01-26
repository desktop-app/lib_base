// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_haptic_linux.h"

#include <QtGui/QGuiApplication>

#include <sigxcpufeedback/sigxcpufeedback.hpp>

namespace base::Platform {
namespace {

using namespace gi::repository;
namespace GObject = gi::repository::GObject;

} // namespace

void Haptic() {
	SigxcpuFeedback::HapticProxy::new_for_bus(
		Gio::BusType::SESSION_,
		Gio::DBusProxyFlags::NONE_,
		"org.sigxcpu.Feedback",
		"/org/sigxcpu/Feedback",
		[=](GObject::Object, Gio::AsyncResult res) {
			auto interface = SigxcpuFeedback::Haptic(
				SigxcpuFeedback::HapticProxy::new_for_bus_finish(
					res,
					nullptr));

			if (!interface) {
				return;
			}

			interface.call_vibrate(
				QGuiApplication::desktopFileName().toStdString(),
				GLib::Variant::new_array({
					GLib::Variant::new_tuple({
						GLib::Variant::new_double(0.2),
						GLib::Variant::new_uint32(5),
					}),
				}),
				nullptr);
		});
}

} // namespace base::Platform
