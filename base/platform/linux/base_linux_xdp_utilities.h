// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include <glibmm.h>

class QWindow;

namespace base::Platform::XDP {

inline constexpr auto kService = "org.freedesktop.portal.Desktop";
inline constexpr auto kObjectPath = "/org/freedesktop/portal/desktop";
inline constexpr auto kRequestInterface = "org.freedesktop.portal.Request";
inline constexpr auto kSettingsInterface = "org.freedesktop.portal.Settings";

Glib::ustring ParentWindowID();
Glib::ustring ParentWindowID(QWindow *window);

std::optional<Glib::VariantBase> ReadSetting(
	const Glib::ustring &group,
	const Glib::ustring &key);

template <typename T>
std::optional<T> ReadSetting(
		const Glib::ustring &group,
		const Glib::ustring &key) {
	try {
		if (const auto value = ReadSetting(group, key)) {
			return value->get_dynamic<T>();
		}
	} catch (...) {
	}
	return std::nullopt;
}

class SettingWatcher {
public:
	SettingWatcher(
		Fn<void(
			const Glib::ustring &,
			const Glib::ustring &,
			const Glib::VariantBase &)> callback);

	template <typename ...Args>
	SettingWatcher(
		Fn<void(
			const Glib::ustring &,
			const Glib::ustring &,
			Args...)> callback)
	: SettingWatcher([=](
			const Glib::ustring &group,
			const Glib::ustring &key,
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
		const Glib::ustring &group,
		const Glib::ustring &key,
		Fn<void(Args...)> callback)
	: SettingWatcher([=](
			const Glib::ustring &group2,
			const Glib::ustring &key2,
			Args &&...value) {
		if (group.raw() == group2.raw() && key.raw() == key2.raw()) {
			callback(std::forward<decltype(value)>(value)...);
		}
	}) {
	}

	template <typename Callback>
	SettingWatcher(
		const Glib::ustring &group,
		const Glib::ustring &key,
		Callback callback)
	: SettingWatcher(group, key, std::function(callback)) {
	}

	~SettingWatcher();

private:
	class Private;
	const std::unique_ptr<Private> _private;
};

} // namespace base::Platform::XDP
