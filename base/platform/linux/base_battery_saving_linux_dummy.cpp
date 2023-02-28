// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_battery_saving_linux.h"

#include "base/battery_saving.h"

namespace base::Platform {
namespace {

class BatterySaving final : public AbstractBatterySaving {
public:
	BatterySaving(Fn<void()> changedCallback);
	~BatterySaving();

	std::optional<bool> enabled() const override;

private:
	Fn<void()> _changedCallback;

};

BatterySaving::BatterySaving(Fn<void()> changedCallback)
: _changedCallback(std::move(changedCallback)) {
}

BatterySaving::~BatterySaving() {
}

std::optional<bool> BatterySaving::enabled() const {
	return std::nullopt;
}

} // namespace

std::unique_ptr<AbstractBatterySaving> CreateBatterySaving(
		Fn<void()> changedCallback) {
	return std::make_unique<BatterySaving>(std::move(changedCallback));
}

} // namespace base::Platform
