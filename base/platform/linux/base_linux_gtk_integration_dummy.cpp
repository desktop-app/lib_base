// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_linux_gtk_integration.h"

namespace base {
namespace Platform {

GtkIntegration::GtkIntegration() {
}

GtkIntegration *GtkIntegration::Instance() {
	return nullptr;
}

void GtkIntegration::prepareEnvironment() {
}

void GtkIntegration::load() {
}

bool GtkIntegration::loaded() const {
	return false;
}

bool GtkIntegration::checkVersion(uint major, uint minor, uint micro) const {
	return false;
}

std::optional<bool> GtkIntegration::getBoolSetting(
		const QString &propertyName) const {
	return std::nullopt;
}

std::optional<int> GtkIntegration::getIntSetting(
		const QString &propertyName) const {
	return std::nullopt;
}

std::optional<QString> GtkIntegration::getStringSetting(
		const QString &propertyName) const {
	return std::nullopt;
}

void GtkIntegration::connectToSetting(
		const QString &propertyName,
		void (*callback)()) {
}

} // namespace Platform
} // namespace base
