// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/expected.h"
#include "base/weak_ptr.h"

#include <glibmm.h>

class QWindow;

namespace base::Platform::XDP {

inline constexpr auto kService = "org.freedesktop.portal.Desktop";
inline constexpr auto kObjectPath = "/org/freedesktop/portal/desktop";
inline constexpr auto kRequestInterface = "org.freedesktop.portal.Request";
inline constexpr auto kSettingsInterface = "org.freedesktop.portal.Settings";

template <typename T>
using Result = expected<T, std::unique_ptr<std::exception>>;

std::string ParentWindowID();
std::string ParentWindowID(QWindow *window);

Result<Glib::VariantBase> ReadSetting(
	const std::string &group,
	const std::string &key);

template <typename T>
Result<T> ReadSetting(
		const std::string &group,
		const std::string &key) {
	try {
		auto value = ReadSetting(group, key);
		if (value) {
			return value->get_dynamic<T>();
		}
		return make_unexpected(std::move(value.error()));
	} catch (std::invalid_argument &e) {
		return make_unexpected(
			std::make_unique<std::invalid_argument>(std::move(e)));
	} catch (std::bad_cast &e) {
		return make_unexpected(std::make_unique<std::bad_cast>(std::move(e)));
	} catch (...) {
		return make_unexpected(std::make_unique<std::exception>());
	}
}

class SettingWatcher : public has_weak_ptr {
public:
	SettingWatcher(
		Fn<void(
			const std::string &,
			const std::string &,
			const Glib::VariantBase &)> callback);

	template <typename ...Args>
	SettingWatcher(
		Fn<void(
			const std::string &,
			const std::string &,
			Args...)> callback)
	: SettingWatcher([=](
			const std::string &group,
			const std::string &key,
			const Glib::VariantBase &value) {
		if constexpr (sizeof...(Args) == 0) {
			callback(group, key);
			return;
		}
		try {
			using Tuple = std::tuple<std::decay_t<Args>...>;
			if constexpr (sizeof...(Args) == 1) {
				using Arg0 = std::tuple_element_t<0, Tuple>;
				callback(group, key, value.get_dynamic<Arg0>());
			} else {
				std::apply(
					callback,
					std::tuple_cat(
						std::forward_as_tuple(group, key),
						value.get_dynamic<Tuple>()));
			}
		} catch (...) {
		}
	}) {
	}

	template <typename Callback>
	SettingWatcher(Callback callback)
	: SettingWatcher(std::function(callback)) {
	}

	template <typename ...Args>
	SettingWatcher(
		const std::string &group,
		const std::string &key,
		Fn<void(Args...)> callback)
	: SettingWatcher([=](
			const std::string &group2,
			const std::string &key2,
			Args &&...value) {
		if (group == group2 && key == key2) {
			callback(std::forward<decltype(value)>(value)...);
		}
	}) {
	}

	template <typename Callback>
	SettingWatcher(
		const std::string &group,
		const std::string &key,
		Callback callback)
	: SettingWatcher(group, key, std::function(callback)) {
	}

	~SettingWatcher();

private:
	class Private;
	const std::unique_ptr<Private> _private;
};

} // namespace base::Platform::XDP
