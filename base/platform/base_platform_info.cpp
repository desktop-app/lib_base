// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/base_platform_info.h"

#include "base/algorithm.h"

namespace base::Platform {
namespace {

constexpr auto kMaxDeviceModelLength = 15;
constexpr auto kMaxGoodDeviceModelLength = 32;

} // namespace

bool IsDeviceModelOk(const QString &model) {
	return !model.isEmpty() && (model.size() <= kMaxDeviceModelLength);
}

QString SimplifyDeviceModel(QString model) {
	return CleanAndSimplify(model.replace(QChar('_'), QString()));
}

QString SimplifyGoodDeviceModel(QString model, std::vector<QString> remove) {
	const auto limit = kMaxGoodDeviceModelLength;
	auto result = QString();
	for (const auto &word : model.split(QChar(' '))) {
		if (ranges::contains(remove, word.toLower())) {
			continue;
		} else if (result.isEmpty()) {
			result = word;
		} else if (result.size() + word.size() + 1 > limit) {
			return result;
		} else {
			result += ' ' + word;
		}
	}
	return result;
}

QString ProductNameToDeviceModel(const QString &productName) {
	if (productName.startsWith("HP ")) {
		// Some special cases for good strings, like HP laptops.
		return SimplifyGoodDeviceModel(
			productName,
			{ "notebook", "desktop", "mobile", "workstation", "pc" });
	} else if (IsDeviceModelOk(productName)) {
		return productName;
	}
	return {};
}

QString FinalizeDeviceModel(QString model) {
	using namespace ::Platform;

	model = std::move(model).trimmed();
	return !model.isEmpty() ? model : IsMac() ? u"Mac"_q : u"Desktop"_q;
}

} // namespace base::Platform
