// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/base_file_utilities.h"

#include "base/platform/base_platform_file_utilities.h"

#include <QtCore/QResource>

namespace base {
namespace {

[[nodiscard]] bool IsBidiControl(ushort code) {
	return (code == 0x061C) // ALM, LRM/RLM, LRE..RLO with PDF, LRI..PDI.
		|| (code == 0x200E || code == 0x200F)
		|| (code >= 0x202A && code <= 0x202E)
		|| (code >= 0x2066 && code <= 0x2069);
}

} // namespace

QString FileNameFromUserString(QString name) {
	// We don't want LTR/RTL mark/embedding/override/isolate chars
	// in filenames, because they introduce a security issue, when
	// an executable "Fil[x]gepj.exe" may look like "Filexe.jpeg".
	const ushort kBad[] = { '/', '\\', '<', '>', ':', '"', '|', '?', '*' };
	for (auto &ch : name) {
		const auto code = ch.unicode();
		if (code < 32
			|| IsBidiControl(code)
			|| ranges::find(kBad, code) != end(kBad)) {
			ch = '_';
		}
	}
	if (name.isEmpty() || name.endsWith(' ') || name.endsWith('.')) {
		name.append('_');
	}
	return Platform::FileNameFromUserString(std::move(name));
}

void RegisterBundledResources(const QString &name) {
	const auto location = Platform::BundledResourcesPath();
	if (!QResource::registerResource(location + '/' + name)) {
		Unexpected("Packed resources not found.");
	}
}

} // namespace base
