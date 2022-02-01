// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

namespace base {

enum class PowerSaveBlockType {
	PreventAppSuspension,
	PreventDisplaySleep,

	kCount,
};

class PowerSaveBlocker final {
public:
	PowerSaveBlocker(PowerSaveBlockType type, const QString &description);
	~PowerSaveBlocker();

	[[nodiscard]] PowerSaveBlockType type() const {
		return _type;
	}
	[[nodiscard]] const QString &description() const {
		return _description;
	}

private:
	const PowerSaveBlockType _type = {};
	const QString _description;

};

} // namespace base
