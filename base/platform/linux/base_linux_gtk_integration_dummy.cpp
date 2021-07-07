// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_linux_gtk_integration.h"

namespace base {
namespace Platform {

class GtkIntegration::Private {
};

GtkIntegration::GtkIntegration() {
}

GtkIntegration::~GtkIntegration() = default;

GtkIntegration *GtkIntegration::Instance() {
	return nullptr;
}

void GtkIntegration::load(const QString &allowedBackends, bool force) {
}

int GtkIntegration::exec(const QString &parentDBusName) {
	return 1;
}

void GtkIntegration::initializeSettings() {
}

bool GtkIntegration::loaded() const {
	return false;
}

QString GtkIntegration::ServiceName() {
	return QString();
}

void GtkIntegration::SetServiceName(const QString &serviceName) {
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
		Fn<void()> callback) {
}

} // namespace Platform
} // namespace base
