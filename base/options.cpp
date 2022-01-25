// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/options.h"

namespace base::options {
namespace details {
namespace {

struct Compare {
	bool operator()(const char *a, const char *b) const noexcept {
		return strcmp(a, b) < 0;
	}
};
using MapType = base::flat_map<const char*, not_null<BasicOption*>, Compare>;

[[nodiscard]] MapType &Map() {
	static auto result = MapType();
	return result;
}

} // namespace

BasicOption::BasicOption(
	const char id[],
	const char name[],
	const char description[],
	ValueType defaultValue)
: _value(defaultValue)
, _defaultValue(std::move(defaultValue))
, _id(QString::fromUtf8(id))
, _name(QString::fromUtf8(name))
, _description(QString::fromUtf8(description)) {
	const auto [i, ok] = Map().emplace(id, this);

	Ensures(ok);
}

void BasicOption::set(ValueType value) {
	Expects(value.index() == _value.index());

	_value = std::move(value);
}

const ValueType &BasicOption::value() const {
	return _value;
}

const ValueType &BasicOption::defaultValue() const {
	return _defaultValue;
}

const QString &BasicOption::id() const {
	return _id;
}

const QString &BasicOption::name() const {
	return _name;
}

const QString &BasicOption::description() const {
	return _description;
}

BasicOption &Lookup(const char id[]) {
	const auto i = Map().find(id);

	Ensures(i != end(Map()));
	return *i->second;
}

} // namespace details
} // namespace base::options
