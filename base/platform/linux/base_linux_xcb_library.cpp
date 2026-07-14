// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_linux_xcb_library.h"

#include "base/platform/linux/base_linux_library.h"

namespace base::Platform::XCB::Library {

void *LoadSymbol(const char *name) {
	static const auto Handle = LoadLibrary("libxcb.so.1", RTLD_NODELETE);
	return Handle ? LoadSymbolGeneric(Handle, name) : nullptr;
}

xcb_connection_t* xcb_connect(
		const char *displayname,
		int *screenp) {
	static const auto real = LoadSymbol<
		xcb_connection_t*(const char*, int*)>("xcb_connect");
	return real ? real(displayname, screenp) : nullptr;
}

void xcb_disconnect(xcb_connection_t *c) {
	static const auto real = LoadSymbol<void(xcb_connection_t*)>(
		"xcb_disconnect");
	real(c);
}

int xcb_connection_has_error(xcb_connection_t *c) {
	static const auto real = LoadSymbol<int(xcb_connection_t*)>(
		"xcb_connection_has_error");
	return real(c);
}

int xcb_get_file_descriptor(xcb_connection_t *c) {
	static const auto real = LoadSymbol<int(xcb_connection_t*)>(
		"xcb_get_file_descriptor");
	return real(c);
}

int xcb_flush(xcb_connection_t *c) {
	static const auto real = LoadSymbol<int(xcb_connection_t*)>(
		"xcb_flush");
	return real(c);
}

uint32_t xcb_generate_id(xcb_connection_t *c) {
	static const auto real = LoadSymbol<uint32_t(xcb_connection_t*)>(
		"xcb_generate_id");
	return real(c);
}

const xcb_setup_t* xcb_get_setup(xcb_connection_t *c) {
	static const auto real = LoadSymbol<
		const xcb_setup_t*(xcb_connection_t*)>("xcb_get_setup");
	return real(c);
}

xcb_screen_iterator_t xcb_setup_roots_iterator(const xcb_setup_t *R) {
	static const auto real = LoadSymbol<
		xcb_screen_iterator_t(const xcb_setup_t*)>(
		"xcb_setup_roots_iterator");
	return real(R);
}

xcb_generic_event_t* xcb_poll_for_event(xcb_connection_t *c) {
	static const auto real = LoadSymbol<
		xcb_generic_event_t*(xcb_connection_t*)>("xcb_poll_for_event");
	return real(c);
}

int xcb_poll_for_reply(
		xcb_connection_t *c,
		unsigned int request,
		void **reply,
		xcb_generic_error_t **error) {
	static const auto real = LoadSymbol<int(
		xcb_connection_t*,
		unsigned int,
		void**,
		xcb_generic_error_t**)>("xcb_poll_for_reply");
	return real(c, request, reply, error);
}

unsigned int xcb_send_request(
		xcb_connection_t *c,
		int flags,
		struct iovec *vector,
		const xcb_protocol_request_t *request) {
	static const auto real = LoadSymbol<unsigned int(
		xcb_connection_t*,
		int,
		struct iovec*,
		const xcb_protocol_request_t*)>("xcb_send_request");
	return real(c, flags, vector, request);
}

void *xcb_wait_for_reply(
		xcb_connection_t *c,
		unsigned int request,
		xcb_generic_error_t **e) {
	static const auto real = LoadSymbol<void*(
		xcb_connection_t*,
		unsigned int,
		xcb_generic_error_t**)>("xcb_wait_for_reply");
	return real(c, request, e);
}

xcb_generic_error_t* xcb_request_check(
		xcb_connection_t *c,
		xcb_void_cookie_t cookie) {
	static const auto real = LoadSymbol<
		xcb_generic_error_t*(xcb_connection_t*, xcb_void_cookie_t)>(
		"xcb_request_check");
	return real(c, cookie);
}

const xcb_query_extension_reply_t* xcb_get_extension_data(
		xcb_connection_t *c,
		xcb_extension_t *ext) {
	static const auto real = LoadSymbol<
		const xcb_query_extension_reply_t*(
			xcb_connection_t*,
			xcb_extension_t*)>("xcb_get_extension_data");
	return real(c, ext);
}

xcb_intern_atom_cookie_t xcb_intern_atom(
		xcb_connection_t *c,
		uint8_t only_if_exists,
		uint16_t name_len,
		const char *name) {
	static const auto real = LoadSymbol<xcb_intern_atom_cookie_t(
		xcb_connection_t*,
		uint8_t,
		uint16_t,
		const char*)>("xcb_intern_atom");
	return real(c, only_if_exists, name_len, name);
}

xcb_intern_atom_reply_t* xcb_intern_atom_reply(
		xcb_connection_t *c,
		xcb_intern_atom_cookie_t cookie,
		xcb_generic_error_t **e) {
	static const auto real = LoadSymbol<xcb_intern_atom_reply_t*(
		xcb_connection_t*,
		xcb_intern_atom_cookie_t,
		xcb_generic_error_t**)>("xcb_intern_atom_reply");
	return real(c, cookie, e);
}

xcb_get_property_cookie_t xcb_get_property(
		xcb_connection_t *c,
		uint8_t _delete,
		xcb_window_t window,
		xcb_atom_t property,
		xcb_atom_t type,
		uint32_t long_offset,
		uint32_t long_length) {
	static const auto real = LoadSymbol<xcb_get_property_cookie_t(
		xcb_connection_t*,
		uint8_t,
		xcb_window_t,
		xcb_atom_t,
		xcb_atom_t,
		uint32_t,
		uint32_t)>("xcb_get_property");
	return real(c, _delete, window, property, type, long_offset, long_length);
}

xcb_get_property_reply_t* xcb_get_property_reply(
		xcb_connection_t *c,
		xcb_get_property_cookie_t cookie,
		xcb_generic_error_t **e) {
	static const auto real = LoadSymbol<xcb_get_property_reply_t*(
		xcb_connection_t*,
		xcb_get_property_cookie_t,
		xcb_generic_error_t**)>("xcb_get_property_reply");
	return real(c, cookie, e);
}

void* xcb_get_property_value(const xcb_get_property_reply_t *R) {
	static const auto real = LoadSymbol<
		void*(const xcb_get_property_reply_t*)>("xcb_get_property_value");
	return real(R);
}

int xcb_get_property_value_length(
		const xcb_get_property_reply_t *R) {
	static const auto real = LoadSymbol<
		int(const xcb_get_property_reply_t*)>(
		"xcb_get_property_value_length");
	return real(R);
}

xcb_get_window_attributes_cookie_t xcb_get_window_attributes(
		xcb_connection_t *c,
		xcb_window_t window) {
	static const auto real = LoadSymbol<
		xcb_get_window_attributes_cookie_t(
			xcb_connection_t*,
			xcb_window_t)>("xcb_get_window_attributes");
	return real(c, window);
}

xcb_get_window_attributes_reply_t* xcb_get_window_attributes_reply(
		xcb_connection_t *c,
		xcb_get_window_attributes_cookie_t cookie,
		xcb_generic_error_t **e) {
	static const auto real = LoadSymbol<
		xcb_get_window_attributes_reply_t*(
			xcb_connection_t*,
			xcb_get_window_attributes_cookie_t,
			xcb_generic_error_t**)>("xcb_get_window_attributes_reply");
	return real(c, cookie, e);
}

xcb_get_input_focus_cookie_t xcb_get_input_focus(
		xcb_connection_t *c) {
	static const auto real = LoadSymbol<
		xcb_get_input_focus_cookie_t(xcb_connection_t*)>(
		"xcb_get_input_focus");
	return real(c);
}

xcb_get_input_focus_reply_t* xcb_get_input_focus_reply(
		xcb_connection_t *c,
		xcb_get_input_focus_cookie_t cookie,
		xcb_generic_error_t **e) {
	static const auto real = LoadSymbol<xcb_get_input_focus_reply_t*(
		xcb_connection_t*,
		xcb_get_input_focus_cookie_t,
		xcb_generic_error_t**)>("xcb_get_input_focus_reply");
	return real(c, cookie, e);
}

xcb_get_selection_owner_cookie_t xcb_get_selection_owner(
		xcb_connection_t *c,
		xcb_atom_t selection) {
	static const auto real = LoadSymbol<
		xcb_get_selection_owner_cookie_t(xcb_connection_t*, xcb_atom_t)>(
		"xcb_get_selection_owner");
	return real(c, selection);
}

xcb_get_selection_owner_reply_t* xcb_get_selection_owner_reply(
		xcb_connection_t *c,
		xcb_get_selection_owner_cookie_t cookie,
		xcb_generic_error_t **e) {
	static const auto real = LoadSymbol<
		xcb_get_selection_owner_reply_t*(
			xcb_connection_t*,
			xcb_get_selection_owner_cookie_t,
			xcb_generic_error_t**)>("xcb_get_selection_owner_reply");
	return real(c, cookie, e);
}

xcb_query_extension_cookie_t xcb_query_extension(
		xcb_connection_t *c,
		uint16_t name_len,
		const char *name) {
	static const auto real = LoadSymbol<xcb_query_extension_cookie_t(
		xcb_connection_t*,
		uint16_t,
		const char*)>("xcb_query_extension");
	return real(c, name_len, name);
}

xcb_query_extension_reply_t* xcb_query_extension_reply(
		xcb_connection_t *c,
		xcb_query_extension_cookie_t cookie,
		xcb_generic_error_t **e) {
	static const auto real = LoadSymbol<xcb_query_extension_reply_t*(
		xcb_connection_t*,
		xcb_query_extension_cookie_t,
		xcb_generic_error_t**)>("xcb_query_extension_reply");
	return real(c, cookie, e);
}

xcb_void_cookie_t xcb_change_property_checked(
		xcb_connection_t *c,
		uint8_t mode,
		xcb_window_t window,
		xcb_atom_t property,
		xcb_atom_t type,
		uint8_t format,
		uint32_t data_len,
		const void *data) {
	static const auto real = LoadSymbol<xcb_void_cookie_t(
		xcb_connection_t*,
		uint8_t,
		xcb_window_t,
		xcb_atom_t,
		xcb_atom_t,
		uint8_t,
		uint32_t,
		const void*)>("xcb_change_property_checked");
	return real(c, mode, window, property, type, format, data_len, data);
}

xcb_void_cookie_t xcb_change_window_attributes_checked(
		xcb_connection_t *c,
		xcb_window_t window,
		uint32_t value_mask,
		const void *value_list) {
	static const auto real = LoadSymbol<xcb_void_cookie_t(
		xcb_connection_t*,
		xcb_window_t,
		uint32_t,
		const void*)>("xcb_change_window_attributes_checked");
	return real(c, window, value_mask, value_list);
}

xcb_void_cookie_t xcb_send_event_checked(
		xcb_connection_t *c,
		uint8_t propagate,
		xcb_window_t destination,
		uint32_t event_mask,
		const char *event) {
	static const auto real = LoadSymbol<xcb_void_cookie_t(
		xcb_connection_t*,
		uint8_t,
		xcb_window_t,
		uint32_t,
		const char*)>("xcb_send_event_checked");
	return real(c, propagate, destination, event_mask, event);
}

xcb_void_cookie_t xcb_map_window_checked(
		xcb_connection_t *c,
		xcb_window_t window) {
	static const auto real = LoadSymbol<
		xcb_void_cookie_t(xcb_connection_t*, xcb_window_t)>(
		"xcb_map_window_checked");
	return real(c, window);
}

xcb_void_cookie_t xcb_configure_window_checked(
		xcb_connection_t *c,
		xcb_window_t window,
		uint16_t value_mask,
		const void *value_list) {
	static const auto real = LoadSymbol<xcb_void_cookie_t(
		xcb_connection_t*,
		xcb_window_t,
		uint16_t,
		const void*)>("xcb_configure_window_checked");
	return real(c, window, value_mask, value_list);
}

xcb_void_cookie_t xcb_force_screen_saver_checked(
		xcb_connection_t *c,
		uint8_t mode) {
	static const auto real = LoadSymbol<
		xcb_void_cookie_t(xcb_connection_t*, uint8_t)>(
		"xcb_force_screen_saver_checked");
	return real(c, mode);
}

} // namespace base::Platform::XCB::Library
