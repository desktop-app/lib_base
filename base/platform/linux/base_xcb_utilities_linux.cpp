// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_xcb_utilities_linux.h"

#include <QtGui/QGuiApplication>
#include <qpa/qplatformnativeinterface.h>

namespace base::Platform::XCB {

xcb_connection_t *GetConnectionFromQt() {
	const auto native = QGuiApplication::platformNativeInterface();
	if (!native) {
		return nullptr;
	}

	return reinterpret_cast<xcb_connection_t*>(
		native->nativeResourceForIntegration(QByteArray("connection")));
}

std::optional<xcb_window_t> GetRootWindowFromQt() {
	const auto native = QGuiApplication::platformNativeInterface();
	if (!native) {
		return std::nullopt;
	}

	return static_cast<xcb_window_t>(reinterpret_cast<quintptr>(
		native->nativeResourceForIntegration(QByteArray("rootwindow"))));;
}

std::optional<xcb_atom_t> GetAtom(
		xcb_connection_t *connection,
		const QString &name) {
	const auto cookie = xcb_intern_atom(
		connection,
		0,
		name.size(),
		name.toUtf8());

	auto reply = xcb_intern_atom_reply(
		connection,
		cookie,
		nullptr);

	if (!reply) {
		return std::nullopt;
	}

	const auto atom = reply->atom;
	free(reply);

	return atom;
}

bool IsExtensionPresent(
		xcb_connection_t *connection,
		xcb_extension_t *ext) {
	const auto reply = xcb_get_extension_data(
		connection,
		ext);

	if (!reply) {
		return false;
	}

	return reply->present;
}

std::vector<xcb_atom_t> GetWMSupported(xcb_connection_t *connection) {
	auto netWmAtoms = std::vector<xcb_atom_t>{};

	const auto root = GetRootWindowFromQt();
	if (!root.has_value()) {
		return netWmAtoms;
	}

	const auto supportedAtom = GetAtom(connection, "_NET_SUPPORTED");
	if (!supportedAtom.has_value()) {
		return netWmAtoms;
	}

	auto offset = 0;
	auto remaining = 0;

	do {
		const auto cookie = xcb_get_property(
			connection,
			false,
			*root,
			*supportedAtom,
			XCB_ATOM_ATOM,
			offset,
			1024);

		auto reply = xcb_get_property_reply(
			connection,
			cookie,
			nullptr);

		if (!reply) {
			break;
		}

		remaining = 0;

		if (reply->type == XCB_ATOM_ATOM && reply->format == 32) {
			const auto len = xcb_get_property_value_length(reply)
				/ sizeof(xcb_atom_t);

			const auto atoms = reinterpret_cast<xcb_atom_t*>(
				xcb_get_property_value(reply));

			const auto s = netWmAtoms.size();
			netWmAtoms.resize(s + len);
			memcpy(netWmAtoms.data() + s, atoms, len * sizeof(xcb_atom_t));

			remaining = reply->bytes_after;
			offset += len;
		}

		free(reply);
	} while (remaining > 0);

	return netWmAtoms;
}

} // namespace base::Platform::XCB
