// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/expected.h"

typedef struct _GDBusConnection GDBusConnection;

namespace base::Platform::DBus {

inline constexpr auto kService = "org.freedesktop.DBus";
inline constexpr auto kObjectPath = "/org/freedesktop/DBus";
inline constexpr auto kInterface = kService;

template <typename T>
using Result = expected<T, std::unique_ptr<std::exception>>;

enum class StartReply {
	Success,
	AlreadyRunning,
};

Result<bool> NameHasOwner(
	const GDBusConnection *connection,
	const std::string &name);

Result<std::vector<std::string>> ListActivatableNames(
	const GDBusConnection *connection);

Result<StartReply> StartServiceByName(
	const GDBusConnection *connection,
	const std::string &name);

void StartServiceByNameAsync(
	const GDBusConnection *connection,
	const std::string &name,
	Fn<void(Fn<Result<StartReply>()>)> callback);

class ServiceWatcher {
public:
	ServiceWatcher(
		const GDBusConnection *connection,
		const std::string &service,
		Fn<void(
			const std::string &,
			const std::string &,
			const std::string &)> callback);

	~ServiceWatcher();

private:
	struct Private;
	std::unique_ptr<Private> _private;
};

} // namespace base::Platform::DBus
