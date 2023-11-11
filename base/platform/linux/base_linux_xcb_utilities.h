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

template <typename T>
using ReplyPointer = std::unique_ptr<T, custom_delete<free>>;

template <typename T>
ReplyPointer<T> MakeReplyPointer(T *reply) {
	return ReplyPointer<T>(reply);
}

std::shared_ptr<CustomConnection> SharedConnection();

xcb_connection_t *GetConnectionFromQt();

std::optional<xcb_timestamp_t> GetTimestamp();

std::optional<xcb_window_t> GetRootWindow(xcb_connection_t *connection);

std::optional<xcb_atom_t> GetAtom(
		xcb_connection_t *connection,
		const QString &name);

bool IsExtensionPresent(
		xcb_connection_t *connection,
		xcb_extension_t *ext);

std::vector<xcb_atom_t> GetWMSupported(
		xcb_connection_t *connection,
		xcb_window_t root);

std::optional<xcb_window_t> GetSupportingWMCheck(
		xcb_connection_t *connection,
		xcb_window_t root);

// convenient API, checks connection for nullptr
bool IsSupportedByWM(xcb_connection_t *connection, const QString &atomName);

class Connection {
public:
	Connection()
	: _qtConnection(GetConnectionFromQt())
	, _customConnection(_qtConnection ? nullptr : SharedConnection()) {
	}

	[[nodiscard]] operator xcb_connection_t*() const {
		return _customConnection ? _customConnection->get() : _qtConnection;
	}

private:
	xcb_connection_t * const _qtConnection = nullptr;
	const std::shared_ptr<CustomConnection> _customConnection;
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
