// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/custom_delete.h"

#include <xcb/xcb.h>

namespace base::Platform::XCB {

using ConnectionPointer = std::unique_ptr<
	xcb_connection_t,
	custom_delete<xcb_disconnect>
>;

class CustomConnection : public ConnectionPointer {
public:
	CustomConnection()
	: ConnectionPointer(xcb_connect(nullptr, nullptr)) {
	}

	[[nodiscard]] operator xcb_connection_t*() const {
		return get();
	}
};

class SharedConnection : public std::shared_ptr<CustomConnection> {
public:
	SharedConnection();

	[[nodiscard]] operator xcb_connection_t*() const {
		return get() ? get()->get() : nullptr;
	}
};

template <typename T>
using EventPointer = std::unique_ptr<T, custom_delete<free>>;

template <typename T>
[[nodiscard]] EventPointer<T> MakeEventPointer(T *event) {
	return EventPointer<T>(event);
}

template <typename T>
using ReplyPointer = std::unique_ptr<T, custom_delete<free>>;

template <typename T>
[[nodiscard]] ReplyPointer<T> MakeReplyPointer(T *reply) {
	return ReplyPointer<T>(reply);
}

template <typename T>
using ErrorPointer = std::unique_ptr<T, custom_delete<free>>;

template <typename T>
[[nodiscard]] ErrorPointer<T> MakeErrorPointer(T *error) {
	return ErrorPointer<T>(error);
}

[[nodiscard]] xcb_connection_t *GetConnectionFromQt();

[[nodiscard]] rpl::lifetime InstallEventHandler(
	xcb_connection_t *connection,
	Fn<void(xcb_generic_event_t*)> handler);

[[nodiscard]] xcb_timestamp_t GetTimestamp(xcb_connection_t *connection);

[[nodiscard]] xcb_window_t GetRootWindow(xcb_connection_t *connection);

[[nodiscard]] xcb_atom_t GetAtom(
	xcb_connection_t *connection,
	const QString &name);

[[nodiscard]] bool IsExtensionPresent(
		xcb_connection_t *connection,
		xcb_extension_t *ext);

[[nodiscard]] std::vector<xcb_atom_t> GetWMSupported(
		xcb_connection_t *connection,
		xcb_window_t root);

[[nodiscard]] xcb_window_t GetSupportingWMCheck(
		xcb_connection_t *connection,
		xcb_window_t root);

[[nodiscard]] bool IsSupportedByWM(
	xcb_connection_t *connection,
	const QString &atomName);

enum class ChangeWindowEventMaskMode {
	Add,
	Remove,
	Replace,
};

[[nodiscard]] rpl::lifetime ChangeWindowEventMask(
	xcb_connection_t *connection,
	xcb_window_t window,
	uint mask,
	ChangeWindowEventMaskMode mode = ChangeWindowEventMaskMode::Add,
	bool revert = true);

class Connection {
public:
	Connection()
	: _qtConnection(GetConnectionFromQt())
	, _sharedConnection(_qtConnection
			? std::optional<SharedConnection>()
			: std::optional<SharedConnection>(std::in_place)) {
	}

	[[nodiscard]] operator xcb_connection_t*() const {
		return _sharedConnection
			? static_cast<xcb_connection_t*>(*_sharedConnection)
			: _qtConnection;
	}

private:
	xcb_connection_t * const _qtConnection = nullptr;
	const std::optional<SharedConnection> _sharedConnection;
};

template <typename Object, auto constructor, auto destructor>
class ObjectWithConnection
	: public std::unique_ptr<Object, custom_delete<destructor>> {
public:
	ObjectWithConnection() {
		if (_connection && !xcb_connection_has_error(_connection)) {
			this->reset(constructor(_connection));
		}
	}

	~ObjectWithConnection() {
		this->reset(nullptr);
	}

private:
	const Connection _connection;
};

} // namespace base::Platform::XCB
