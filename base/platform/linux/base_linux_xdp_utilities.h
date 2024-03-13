// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/expected.h"
#include "base/weak_ptr.h"

#include <glib/glib.hpp>

#if __has_include(<glibmm.h>)
#include <glibmm.h>
#endif // __has_include(<glibmm.h>)

class QWindow;

namespace base::Platform::XDP {

inline constexpr auto kService = "org.freedesktop.portal.Desktop";
inline constexpr auto kObjectPath = "/org/freedesktop/portal/desktop";
inline constexpr auto kRequestInterface = "org.freedesktop.portal.Request";
inline constexpr auto kSettingsInterface = "org.freedesktop.portal.Settings";

std::string ParentWindowID();
std::string ParentWindowID(QWindow *window);

gi::result<gi::repository::GLib::Variant> ReadSetting(
	const std::string &group,
	const std::string &key);

#if __has_include(<glibmm.h>)
template <typename T>
gi::result<T> ReadSetting(
		const std::string &group,
		const std::string &key) {
	try {
		auto value = ReadSetting(group, key);
		if (value) {
			return Glib::wrap(value->gobj_copy_()).get_dynamic<T>();
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
#endif

class SettingWatcher : public has_weak_ptr {
public:
	SettingWatcher(
		Fn<void(
			const std::string &,
			const std::string &,
			gi::repository::GLib::Variant)> callback);

	SettingWatcher(
		const std::string &group,
		const std::string &key,
		Fn<void(gi::repository::GLib::Variant)> callback)
	: SettingWatcher([=](
			const std::string &group2,
			const std::string &key2,
			gi::repository::GLib::Variant value) {
		if (group == group2 && key == key2) {
			callback(value);
		}
	}) {
	}

	SettingWatcher(
		const std::string &group,
		const std::string &key,
		Fn<void()> callback)
	: SettingWatcher(group, key, [=](gi::repository::GLib::Variant) {
		callback();
	}) {
	}

#if __has_include(<glibmm.h>)
	template <typename ...Args>
	SettingWatcher(
		Fn<void(
			const std::string &,
			const std::string &,
			Args...)> callback)
	: SettingWatcher([=](
			const std::string &group,
			const std::string &key,
			gi::repository::GLib::Variant value) {
		if constexpr (sizeof...(Args) == 0) {
			callback(group, key);
			return;
		}
		try {
			using Tuple = std::tuple<std::decay_t<Args>...>;
			if constexpr (sizeof...(Args) == 1) {
				using Arg0 = std::tuple_element_t<0, Tuple>;
				callback(
					group,
					key,
					Glib::wrap(value.gobj_copy_()).get_dynamic<Arg0>());
			} else {
				std::apply(
					callback,
					std::tuple_cat(
						std::forward_as_tuple(group, key),
						Glib::wrap(value.gobj_copy_()).get_dynamic<Tuple>()));
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
#endif // __has_include(<glibmm.h>)

	~SettingWatcher();

private:
	class Private;
	const std::unique_ptr<Private> _private;
};

} // namespace base::Platform::XDP
