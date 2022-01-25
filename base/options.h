// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/variant.h"
#include "base/required.h"

namespace base::options {
namespace details {

using ValueType = std::variant<bool, int, QString>;

class BasicOption {
public:
	BasicOption(
		const char id[],
		const char name[],
		const char description[],
		ValueType defaultValue);
	BasicOption(const BasicOption&) = delete;
	BasicOption operator=(const BasicOption&) = delete;

	void set(ValueType value);
	[[nodiscard]] const ValueType &value() const;
	[[nodiscard]] const ValueType &defaultValue() const;

	[[nodiscard]] const QString &id() const;
	[[nodiscard]] const QString &name() const;
	[[nodiscard]] const QString &description() const;

private:
	ValueType _value;
	ValueType _defaultValue;

	QString _id;
	QString _name;
	QString _description;

};

[[nodiscard]] BasicOption &Lookup(const char name[]);

} // namespace details

template <typename Type>
struct descriptor {
	required<const char*> id;
	const char *name = "";
	const char *description = "";
	Type defaultValue = Type();
};

template <typename Type>
class option final : details::BasicOption {
public:
	option(descriptor<Type> &&fields)
	: BasicOption(
		fields.id,
		fields.name,
		fields.description,
		std::move(fields.defaultValue)) {
	}

	using BasicOption::id;
	using BasicOption::name;
	using BasicOption::description;

	void set(Type value) {
		BasicOption::set(std::move(value));
	}
	[[nodiscard]] Type value() const {
		return v::get<Type>(BasicOption::value());
	}
	[[nodiscard]] Type defaultValue() const {
		return v::get<Type>(BasicOption::value());
	}

	[[nodiscard]] static option &Wrap(BasicOption &that) {
		Expects(v::is<Type>(that.value()));

		return static_cast<option&>(that);
	}
};

using toggle = option<bool>;

template <typename Type>
[[nodiscard]] inline Type value(const char id[]) {
	return v::get<Type>(details::Lookup(id).value());
}

template <typename Type>
[[nodiscard]] inline option<Type> &lookup(const char id[]) {
	return option<Type>::Wrap(details::Lookup(id));
}

} // namespace base::options
