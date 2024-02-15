// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

class QWindow;

namespace base {
namespace Platform {

class WaylandIntegration {
public:
	WaylandIntegration();
	~WaylandIntegration();

	[[nodiscard]] static WaylandIntegration *Instance();

	void preventDisplaySleep(not_null<QWindow*> window, bool prevent);

private:
	struct Private;
	const std::unique_ptr<Private> _private;
};

} // namespace Platform
} // namespace base
