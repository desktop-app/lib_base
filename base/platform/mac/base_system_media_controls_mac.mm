// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/base_platform_system_media_controls.h"

#include <QtWidgets/QWidget>

namespace base::Platform {

struct SystemMediaControls::Private {
};

SystemMediaControls::SystemMediaControls()
: _private(std::make_unique<Private>()) {
}

SystemMediaControls::~SystemMediaControls() {
}

bool SystemMediaControls::init(std::optional<QWidget*> parent) {
	return true;
}

void SystemMediaControls::setEnabled(bool enabled) {
}

void SystemMediaControls::setIsNextEnabled(bool value) {
}

void SystemMediaControls::setIsPreviousEnabled(bool value) {
}

void SystemMediaControls::setIsPlayPauseEnabled(bool value) {
}

void SystemMediaControls::setIsStopEnabled(bool value) {
}

void SystemMediaControls::setPlaybackStatus(PlaybackStatus status) {
}

void SystemMediaControls::setTitle(const QString &title) {
}

void SystemMediaControls::setArtist(const QString &artist) {
}

void SystemMediaControls::setThumbnail(const QImage &thumbnail) {
}

void SystemMediaControls::setDuration(int duration) {
}

void SystemMediaControls::setPosition(int position) {
}

void SystemMediaControls::clearThumbnail() {
}

void SystemMediaControls::clearMetadata() {
}

void SystemMediaControls::updateDisplay() {
}

auto SystemMediaControls::commandRequests() const
-> rpl::producer<SystemMediaControls::Command> {
	return _commandRequests.events();
}

rpl::producer<float64> SystemMediaControls::seekRequests() const {
	return rpl::never<float64>();
}

bool SystemMediaControls::seekingSupported() const {
	return true;
}

bool SystemMediaControls::Supported() {
	if (@available(macOS 10.12.2, *)) {
		return true;
	}
	return false;
}

} // namespace base::Platform
