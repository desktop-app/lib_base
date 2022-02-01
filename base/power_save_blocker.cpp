// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/power_save_blocker.h"

#include "base/platform/base_platform_power_save_blocker.h"

#include <QtCore/QMutex>

namespace base {
namespace {

struct BlockersSet {
	flat_set<not_null<PowerSaveBlocker*>> list;
	QString description;
};

flat_map<PowerSaveBlockType, BlockersSet> Blockers;
QMutex Mutex;

void Add(not_null<PowerSaveBlocker*> blocker) {
	const auto lock = QMutexLocker(&Mutex);
	auto &set = Blockers[blocker->type()];
	if (set.list.empty()) {
		set.description = blocker->description();
		Platform::BlockPowerSave(blocker->type(), set.description);
	}
	set.list.emplace(blocker);
}

void Remove(not_null<PowerSaveBlocker*> blocker) {
	const auto lock = QMutexLocker(&Mutex);
	auto &set = Blockers[blocker->type()];
	set.list.remove(blocker);
	if (set.description != blocker->description()) {
		return;
	}
	for (const auto &another : set.list) {
		if (another->description() == set.description) {
			return;
		}
	}
	Platform::UnblockPowerSave(blocker->type());
	if (!set.list.empty()) {
		set.description = set.list.front()->description();
		Platform::BlockPowerSave(blocker->type(), set.description);
	}
}

} // namespace

PowerSaveBlocker::PowerSaveBlocker(
	PowerSaveBlockType type,
	const QString &description)
: _type(type)
, _description(description) {
	Add(this);
}

PowerSaveBlocker::~PowerSaveBlocker() {
	Remove(this);
}

} // namespace base
