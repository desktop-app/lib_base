// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/battery_saving.h"

namespace base {

BatterySaving::BatterySaving()
: _helper(Platform::CreateBatterySaving([=] {
	_value = _helper->enabled();
}))
, _value(_helper->enabled()) {
}

BatterySaving::~BatterySaving() = default;

std::optional<bool> BatterySaving::enabled() const {
	return _value.current();
}

rpl::producer<bool> BatterySaving::value() const {
	return _value.value() | rpl::map([=](std::optional<bool> maybe) {
		return maybe.value_or(false);
	});
}

} // namespace base
