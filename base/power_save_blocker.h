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

// Description is a universal template so you don't construct QStrings.
template <typename Description>
void UpdatePowerSaveBlocker(
		std::unique_ptr<PowerSaveBlocker> &blocker,
		bool block,
		PowerSaveBlockType type,
		Description &&description) {
	if (block && !blocker) {
		blocker = std::make_unique<PowerSaveBlocker>(
			type,
			std::forward<Description>(description));
	} else if (!block && blocker) {
		blocker = nullptr;
	}
}

} // namespace base
