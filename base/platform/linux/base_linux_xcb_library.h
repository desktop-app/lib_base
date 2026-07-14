// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include <stddef.h>
#include <stdint.h>

// Declarations from the xcb headers (X11 license), so that there is
// no build-time dependency on libxcb. These mirror the X11 wire
// protocol, which is frozen, so they can never change. This header
// cannot be included in one translation unit with the real headers.
extern "C" {

struct iovec;

typedef struct xcb_connection_t xcb_connection_t;

typedef struct xcb_extension_t {
	const char *name;
	int global_id;
} xcb_extension_t;

typedef struct {
	size_t count;
	xcb_extension_t *ext;
	uint8_t opcode;
	uint8_t isvoid;
} xcb_protocol_request_t;

enum xcb_send_request_flags_t {
	XCB_REQUEST_CHECKED = 1 << 0,
	XCB_REQUEST_RAW = 1 << 1,
	XCB_REQUEST_DISCARD_REPLY = 1 << 2,
	XCB_REQUEST_REPLY_FDS = 1 << 3
};

typedef uint32_t xcb_window_t;
typedef uint32_t xcb_colormap_t;
typedef uint32_t xcb_atom_t;
typedef uint32_t xcb_drawable_t;
typedef uint32_t xcb_visualid_t;
typedef uint32_t xcb_timestamp_t;
typedef uint32_t xcb_keysym_t;
typedef uint8_t xcb_keycode_t;
typedef uint8_t xcb_button_t;

#define XCB_NONE 0L
#define XCB_COPY_FROM_PARENT 0L
#define XCB_CURRENT_TIME 0L
#define XCB_NO_SYMBOL 0L
#define XCB_KEY_PRESS 2
#define XCB_KEY_RELEASE 3
#define XCB_BUTTON_PRESS 4
#define XCB_BUTTON_RELEASE 5
#define XCB_PROPERTY_NOTIFY 28
#define XCB_CLIENT_MESSAGE 33

typedef enum xcb_atom_enum_t {
    XCB_ATOM_NONE = 0,
    XCB_ATOM_ANY = 0,
    XCB_ATOM_PRIMARY = 1,
    XCB_ATOM_SECONDARY = 2,
    XCB_ATOM_ARC = 3,
    XCB_ATOM_ATOM = 4,
    XCB_ATOM_BITMAP = 5,
    XCB_ATOM_CARDINAL = 6,
    XCB_ATOM_COLORMAP = 7,
    XCB_ATOM_CURSOR = 8,
    XCB_ATOM_CUT_BUFFER0 = 9,
    XCB_ATOM_CUT_BUFFER1 = 10,
    XCB_ATOM_CUT_BUFFER2 = 11,
    XCB_ATOM_CUT_BUFFER3 = 12,
    XCB_ATOM_CUT_BUFFER4 = 13,
    XCB_ATOM_CUT_BUFFER5 = 14,
    XCB_ATOM_CUT_BUFFER6 = 15,
    XCB_ATOM_CUT_BUFFER7 = 16,
    XCB_ATOM_DRAWABLE = 17,
    XCB_ATOM_FONT = 18,
    XCB_ATOM_INTEGER = 19,
    XCB_ATOM_PIXMAP = 20,
    XCB_ATOM_POINT = 21,
    XCB_ATOM_RECTANGLE = 22,
    XCB_ATOM_RESOURCE_MANAGER = 23,
    XCB_ATOM_RGB_COLOR_MAP = 24,
    XCB_ATOM_RGB_BEST_MAP = 25,
    XCB_ATOM_RGB_BLUE_MAP = 26,
    XCB_ATOM_RGB_DEFAULT_MAP = 27,
    XCB_ATOM_RGB_GRAY_MAP = 28,
    XCB_ATOM_RGB_GREEN_MAP = 29,
    XCB_ATOM_RGB_RED_MAP = 30,
    XCB_ATOM_STRING = 31,
    XCB_ATOM_VISUALID = 32,
    XCB_ATOM_WINDOW = 33,
    XCB_ATOM_WM_COMMAND = 34,
    XCB_ATOM_WM_HINTS = 35,
    XCB_ATOM_WM_CLIENT_MACHINE = 36,
    XCB_ATOM_WM_ICON_NAME = 37,
    XCB_ATOM_WM_ICON_SIZE = 38,
    XCB_ATOM_WM_NAME = 39,
    XCB_ATOM_WM_NORMAL_HINTS = 40,
    XCB_ATOM_WM_SIZE_HINTS = 41,
    XCB_ATOM_WM_ZOOM_HINTS = 42,
    XCB_ATOM_MIN_SPACE = 43,
    XCB_ATOM_NORM_SPACE = 44,
    XCB_ATOM_MAX_SPACE = 45,
    XCB_ATOM_END_SPACE = 46,
    XCB_ATOM_SUPERSCRIPT_X = 47,
    XCB_ATOM_SUPERSCRIPT_Y = 48,
    XCB_ATOM_SUBSCRIPT_X = 49,
    XCB_ATOM_SUBSCRIPT_Y = 50,
    XCB_ATOM_UNDERLINE_POSITION = 51,
    XCB_ATOM_UNDERLINE_THICKNESS = 52,
    XCB_ATOM_STRIKEOUT_ASCENT = 53,
    XCB_ATOM_STRIKEOUT_DESCENT = 54,
    XCB_ATOM_ITALIC_ANGLE = 55,
    XCB_ATOM_X_HEIGHT = 56,
    XCB_ATOM_QUAD_WIDTH = 57,
    XCB_ATOM_WEIGHT = 58,
    XCB_ATOM_POINT_SIZE = 59,
    XCB_ATOM_RESOLUTION = 60,
    XCB_ATOM_COPYRIGHT = 61,
    XCB_ATOM_NOTICE = 62,
    XCB_ATOM_FONT_NAME = 63,
    XCB_ATOM_FAMILY_NAME = 64,
    XCB_ATOM_FULL_NAME = 65,
    XCB_ATOM_CAP_HEIGHT = 66,
    XCB_ATOM_WM_CLASS = 67,
    XCB_ATOM_WM_TRANSIENT_FOR = 68
} xcb_atom_enum_t;

typedef enum xcb_prop_mode_t {
    XCB_PROP_MODE_REPLACE = 0,
/**< Discard the previous property value and store the new data. */

    XCB_PROP_MODE_PREPEND = 1,
/**< Insert the new data before the beginning of existing data. The `format` must
match existing property value. If the property is undefined, it is treated as
defined with the correct type and format with zero-length data. */

    XCB_PROP_MODE_APPEND = 2
/**< Insert the new data after the beginning of existing data. The `format` must
match existing property value. If the property is undefined, it is treated as
defined with the correct type and format with zero-length data. */

} xcb_prop_mode_t;

typedef enum xcb_cw_t {
    XCB_CW_BACK_PIXMAP = 1,
/**< Overrides the default background-pixmap. The background pixmap and window must
have the same root and same depth. Any size pixmap can be used, although some
sizes may be faster than others.

If `XCB_BACK_PIXMAP_NONE` is specified, the window has no defined background.
The server may fill the contents with the previous screen contents or with
contents of its own choosing.

If `XCB_BACK_PIXMAP_PARENT_RELATIVE` is specified, the parent's background is
used, but the window must have the same depth as the parent (or a Match error
results).   The parent's background is tracked, and the current version is
used each time the window background is required. */

    XCB_CW_BACK_PIXEL = 2,
/**< Overrides `BackPixmap`. A pixmap of undefined size filled with the specified
background pixel is used for the background. Range-checking is not performed,
the background pixel is truncated to the appropriate number of bits. */

    XCB_CW_BORDER_PIXMAP = 4,
/**< Overrides the default border-pixmap. The border pixmap and window must have the
same root and the same depth. Any size pixmap can be used, although some sizes
may be faster than others.

The special value `XCB_COPY_FROM_PARENT` means the parent's border pixmap is
copied (subsequent changes to the parent's border attribute do not affect the
child), but the window must have the same depth as the parent. */

    XCB_CW_BORDER_PIXEL = 8,
/**< Overrides `BorderPixmap`. A pixmap of undefined size filled with the specified
border pixel is used for the border. Range checking is not performed on the
border-pixel value, it is truncated to the appropriate number of bits. */

    XCB_CW_BIT_GRAVITY = 16,
/**< Defines which region of the window should be retained if the window is resized. */

    XCB_CW_WIN_GRAVITY = 32,
/**< Defines how the window should be repositioned if the parent is resized (see
`ConfigureWindow`). */

    XCB_CW_BACKING_STORE = 64,
/**< A backing-store of `WhenMapped` advises the server that maintaining contents of
obscured regions when the window is mapped would be beneficial. A backing-store
of `Always` advises the server that maintaining contents even when the window
is unmapped would be beneficial. In this case, the server may generate an
exposure event when the window is created. A value of `NotUseful` advises the
server that maintaining contents is unnecessary, although a server may still
choose to maintain contents while the window is mapped. Note that if the server
maintains contents, then the server should maintain complete contents not just
the region within the parent boundaries, even if the window is larger than its
parent. While the server maintains contents, exposure events will not normally
be generated, but the server may stop maintaining contents at any time. */

    XCB_CW_BACKING_PLANES = 128,
/**< The backing-planes indicates (with bits set to 1) which bit planes of the
window hold dynamic data that must be preserved in backing-stores and during
save-unders. */

    XCB_CW_BACKING_PIXEL = 256,
/**< The backing-pixel specifies what value to use in planes not covered by
backing-planes. The server is free to save only the specified bit planes in the
backing-store or save-under and regenerate the remaining planes with the
specified pixel value. Any bits beyond the specified depth of the window in
these values are simply ignored. */

    XCB_CW_OVERRIDE_REDIRECT = 512,
/**< The override-redirect specifies whether map and configure requests on this
window should override a SubstructureRedirect on the parent, typically to
inform a window manager not to tamper with the window. */

    XCB_CW_SAVE_UNDER = 1024,
/**< If 1, the server is advised that when this window is mapped, saving the
contents of windows it obscures would be beneficial. */

    XCB_CW_EVENT_MASK = 2048,
/**< The event-mask defines which events the client is interested in for this window
(or for some event types, inferiors of the window). */

    XCB_CW_DONT_PROPAGATE = 4096,
/**< The do-not-propagate-mask defines which events should not be propagated to
ancestor windows when no client has the event type selected in this window. */

    XCB_CW_COLORMAP = 8192,
/**< The colormap specifies the colormap that best reflects the true colors of the window. Servers
capable of supporting multiple hardware colormaps may use this information, and window man-
agers may use it for InstallColormap requests. The colormap must have the same visual type
and root as the window (or a Match error results). If CopyFromParent is specified, the parent's
colormap is copied (subsequent changes to the parent's colormap attribute do not affect the child).
However, the window must have the same visual type as the parent (or a Match error results),
and the parent must not have a colormap of None (or a Match error results). For an explanation
of None, see FreeColormap request. The colormap is copied by sharing the colormap object
between the child and the parent, not by making a complete copy of the colormap contents. */

    XCB_CW_CURSOR = 16384
/**< If a cursor is specified, it will be used whenever the pointer is in the window. If None is speci-
fied, the parent's cursor will be used when the pointer is in the window, and any change in the
parent's cursor will cause an immediate change in the displayed cursor. */

} xcb_cw_t;

typedef enum xcb_event_mask_t {
    XCB_EVENT_MASK_NO_EVENT = 0,
    XCB_EVENT_MASK_KEY_PRESS = 1,
    XCB_EVENT_MASK_KEY_RELEASE = 2,
    XCB_EVENT_MASK_BUTTON_PRESS = 4,
    XCB_EVENT_MASK_BUTTON_RELEASE = 8,
    XCB_EVENT_MASK_ENTER_WINDOW = 16,
    XCB_EVENT_MASK_LEAVE_WINDOW = 32,
    XCB_EVENT_MASK_POINTER_MOTION = 64,
    XCB_EVENT_MASK_POINTER_MOTION_HINT = 128,
    XCB_EVENT_MASK_BUTTON_1_MOTION = 256,
    XCB_EVENT_MASK_BUTTON_2_MOTION = 512,
    XCB_EVENT_MASK_BUTTON_3_MOTION = 1024,
    XCB_EVENT_MASK_BUTTON_4_MOTION = 2048,
    XCB_EVENT_MASK_BUTTON_5_MOTION = 4096,
    XCB_EVENT_MASK_BUTTON_MOTION = 8192,
    XCB_EVENT_MASK_KEYMAP_STATE = 16384,
    XCB_EVENT_MASK_EXPOSURE = 32768,
    XCB_EVENT_MASK_VISIBILITY_CHANGE = 65536,
    XCB_EVENT_MASK_STRUCTURE_NOTIFY = 131072,
    XCB_EVENT_MASK_RESIZE_REDIRECT = 262144,
    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY = 524288,
    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT = 1048576,
    XCB_EVENT_MASK_FOCUS_CHANGE = 2097152,
    XCB_EVENT_MASK_PROPERTY_CHANGE = 4194304,
    XCB_EVENT_MASK_COLOR_MAP_CHANGE = 8388608,
    XCB_EVENT_MASK_OWNER_GRAB_BUTTON = 16777216
} xcb_event_mask_t;

typedef enum xcb_input_focus_t {
    XCB_INPUT_FOCUS_NONE = 0,
/**< The focus reverts to `XCB_NONE`, so no window will have the input focus. */

    XCB_INPUT_FOCUS_POINTER_ROOT = 1,
/**< The focus reverts to `XCB_POINTER_ROOT` respectively. When the focus reverts,
FocusIn and FocusOut events are generated, but the last-focus-change time is
not changed. */

    XCB_INPUT_FOCUS_PARENT = 2,
/**< The focus reverts to the parent (or closest viewable ancestor) and the new
revert_to value is `XCB_INPUT_FOCUS_NONE`. */

    XCB_INPUT_FOCUS_FOLLOW_KEYBOARD = 3
/**< NOT YET DOCUMENTED. Only relevant for the xinput extension. */

} xcb_input_focus_t;

typedef enum xcb_config_window_t {
    XCB_CONFIG_WINDOW_X = 1,
    XCB_CONFIG_WINDOW_Y = 2,
    XCB_CONFIG_WINDOW_WIDTH = 4,
    XCB_CONFIG_WINDOW_HEIGHT = 8,
    XCB_CONFIG_WINDOW_BORDER_WIDTH = 16,
    XCB_CONFIG_WINDOW_SIBLING = 32,
    XCB_CONFIG_WINDOW_STACK_MODE = 64
} xcb_config_window_t;

typedef enum xcb_stack_mode_t {
    XCB_STACK_MODE_ABOVE = 0,
    XCB_STACK_MODE_BELOW = 1,
    XCB_STACK_MODE_TOP_IF = 2,
    XCB_STACK_MODE_BOTTOM_IF = 3,
    XCB_STACK_MODE_OPPOSITE = 4
} xcb_stack_mode_t;

typedef enum xcb_image_order_t {
    XCB_IMAGE_ORDER_LSB_FIRST = 0,
    XCB_IMAGE_ORDER_MSB_FIRST = 1
} xcb_image_order_t;

typedef enum xcb_screen_saver_t {
    XCB_SCREEN_SAVER_RESET = 0,
    XCB_SCREEN_SAVER_ACTIVE = 1
} xcb_screen_saver_t;

typedef enum xcb_mod_mask_t {
    XCB_MOD_MASK_SHIFT = 1,
    XCB_MOD_MASK_LOCK = 2,
    XCB_MOD_MASK_CONTROL = 4,
    XCB_MOD_MASK_1 = 8,
    XCB_MOD_MASK_2 = 16,
    XCB_MOD_MASK_3 = 32,
    XCB_MOD_MASK_4 = 64,
    XCB_MOD_MASK_5 = 128,
    XCB_MOD_MASK_ANY = 32768
} xcb_mod_mask_t;

typedef struct {
    unsigned int sequence;  /**< Sequence number */
} xcb_void_cookie_t;

typedef struct {
    uint8_t   response_type;  /**< Type of the response */
    uint8_t  pad0;           /**< Padding */
    uint16_t sequence;       /**< Sequence number */
    uint32_t pad[7];         /**< Padding */
    uint32_t full_sequence;  /**< full sequence */
} xcb_generic_event_t;

typedef struct {
    uint8_t   response_type;  /**< Type of the response */
    uint8_t   error_code;     /**< Error code */
    uint16_t sequence;       /**< Sequence number */
    uint32_t resource_id;     /** < Resource ID for requests with side effects only */
    uint16_t minor_code;      /** < Minor opcode of the failed request */
    uint8_t major_code;       /** < Major opcode of the failed request */
    uint8_t pad0;
    uint32_t pad[5];         /**< Padding */
    uint32_t full_sequence;  /**< full sequence */
} xcb_generic_error_t;

typedef struct xcb_screen_t {
    xcb_window_t   root;
    xcb_colormap_t default_colormap;
    uint32_t       white_pixel;
    uint32_t       black_pixel;
    uint32_t       current_input_masks;
    uint16_t       width_in_pixels;
    uint16_t       height_in_pixels;
    uint16_t       width_in_millimeters;
    uint16_t       height_in_millimeters;
    uint16_t       min_installed_maps;
    uint16_t       max_installed_maps;
    xcb_visualid_t root_visual;
    uint8_t        backing_stores;
    uint8_t        save_unders;
    uint8_t        root_depth;
    uint8_t        allowed_depths_len;
} xcb_screen_t;

typedef struct xcb_screen_iterator_t {
    xcb_screen_t *data;
    int           rem;
    int           index;
} xcb_screen_iterator_t;

typedef struct xcb_setup_t {
    uint8_t       status;
    uint8_t       pad0;
    uint16_t      protocol_major_version;
    uint16_t      protocol_minor_version;
    uint16_t      length;
    uint32_t      release_number;
    uint32_t      resource_id_base;
    uint32_t      resource_id_mask;
    uint32_t      motion_buffer_size;
    uint16_t      vendor_len;
    uint16_t      maximum_request_length;
    uint8_t       roots_len;
    uint8_t       pixmap_formats_len;
    uint8_t       image_byte_order;
    uint8_t       bitmap_format_bit_order;
    uint8_t       bitmap_format_scanline_unit;
    uint8_t       bitmap_format_scanline_pad;
    xcb_keycode_t min_keycode;
    xcb_keycode_t max_keycode;
    uint8_t       pad1[4];
} xcb_setup_t;

typedef union xcb_client_message_data_t {
    uint8_t  data8[20];
    uint16_t data16[10];
    uint32_t data32[5];
} xcb_client_message_data_t;

typedef struct xcb_client_message_event_t {
    uint8_t                   response_type;
    uint8_t                   format;
    uint16_t                  sequence;
    xcb_window_t              window;
    xcb_atom_t                type;
    xcb_client_message_data_t data;
} xcb_client_message_event_t;

typedef struct xcb_property_notify_event_t {
    uint8_t         response_type;
    uint8_t         pad0;
    uint16_t        sequence;
    xcb_window_t    window;
    xcb_atom_t      atom;
    xcb_timestamp_t time;
    uint8_t         state;
    uint8_t         pad1[3];
} xcb_property_notify_event_t;

typedef struct xcb_key_press_event_t {
    uint8_t         response_type;
    xcb_keycode_t   detail;
    uint16_t        sequence;
    xcb_timestamp_t time;
    xcb_window_t    root;
    xcb_window_t    event;
    xcb_window_t    child;
    int16_t         root_x;
    int16_t         root_y;
    int16_t         event_x;
    int16_t         event_y;
    uint16_t        state;
    uint8_t         same_screen;
    uint8_t         pad0;
} xcb_key_press_event_t;

typedef struct xcb_button_press_event_t {
    uint8_t         response_type;
    xcb_button_t    detail;
    uint16_t        sequence;
    xcb_timestamp_t time;
    xcb_window_t    root;
    xcb_window_t    event;
    xcb_window_t    child;
    int16_t         root_x;
    int16_t         root_y;
    int16_t         event_x;
    int16_t         event_y;
    uint16_t        state;
    uint8_t         same_screen;
    uint8_t         pad0;
} xcb_button_press_event_t;

typedef struct xcb_intern_atom_cookie_t {
    unsigned int sequence;
} xcb_intern_atom_cookie_t;

typedef struct xcb_intern_atom_reply_t {
    uint8_t    response_type;
    uint8_t    pad0;
    uint16_t   sequence;
    uint32_t   length;
    xcb_atom_t atom;
} xcb_intern_atom_reply_t;

typedef struct xcb_get_property_cookie_t {
    unsigned int sequence;
} xcb_get_property_cookie_t;

typedef struct xcb_get_property_reply_t {
    uint8_t    response_type;
    uint8_t    format;
    uint16_t   sequence;
    uint32_t   length;
    xcb_atom_t type;
    uint32_t   bytes_after;
    uint32_t   value_len;
    uint8_t    pad0[12];
} xcb_get_property_reply_t;

typedef struct xcb_get_window_attributes_cookie_t {
    unsigned int sequence;
} xcb_get_window_attributes_cookie_t;

typedef struct xcb_get_window_attributes_reply_t {
    uint8_t        response_type;
    uint8_t        backing_store;
    uint16_t       sequence;
    uint32_t       length;
    xcb_visualid_t visual;
    uint16_t       _class;
    uint8_t        bit_gravity;
    uint8_t        win_gravity;
    uint32_t       backing_planes;
    uint32_t       backing_pixel;
    uint8_t        save_under;
    uint8_t        map_is_installed;
    uint8_t        map_state;
    uint8_t        override_redirect;
    xcb_colormap_t colormap;
    uint32_t       all_event_masks;
    uint32_t       your_event_mask;
    uint16_t       do_not_propagate_mask;
    uint8_t        pad0[2];
} xcb_get_window_attributes_reply_t;

typedef struct xcb_get_input_focus_cookie_t {
    unsigned int sequence;
} xcb_get_input_focus_cookie_t;

typedef struct xcb_get_input_focus_reply_t {
    uint8_t      response_type;
    uint8_t      revert_to;
    uint16_t     sequence;
    uint32_t     length;
    xcb_window_t focus;
} xcb_get_input_focus_reply_t;

typedef struct xcb_get_selection_owner_cookie_t {
    unsigned int sequence;
} xcb_get_selection_owner_cookie_t;

typedef struct xcb_get_selection_owner_reply_t {
    uint8_t      response_type;
    uint8_t      pad0;
    uint16_t     sequence;
    uint32_t     length;
    xcb_window_t owner;
} xcb_get_selection_owner_reply_t;

typedef struct xcb_query_extension_cookie_t {
    unsigned int sequence;
} xcb_query_extension_cookie_t;

typedef struct xcb_query_extension_reply_t {
    uint8_t  response_type;
    uint8_t  pad0;
    uint16_t sequence;
    uint32_t length;
    uint8_t  present;
    uint8_t  major_opcode;
    uint8_t  first_event;
    uint8_t  first_error;
} xcb_query_extension_reply_t;

} // extern "C"

namespace base::Platform::XCB::Library {

// Resolves a libxcb symbol without a link-time dependency on it.
[[nodiscard]] void *LoadSymbol(const char *name);

template <typename Function>
[[nodiscard]] Function *LoadSymbol(const char *name) {
	return reinterpret_cast<Function*>(LoadSymbol(name));
}

// Lazily resolved forwarders for the used libxcb functions. Each one
// resolves the real symbol on the first call. The connection is
// expected to be checked before calling any of these, so a missing
// libxcb ends up with a null connection (xcb_connect checks for the
// missing library itself) and the forwarders are never reached.
xcb_connection_t* xcb_connect(
		const char *displayname,
		int *screenp);
void xcb_disconnect(xcb_connection_t *c);
int xcb_connection_has_error(xcb_connection_t *c);
int xcb_get_file_descriptor(xcb_connection_t *c);
int xcb_flush(xcb_connection_t *c);
uint32_t xcb_generate_id(xcb_connection_t *c);
const xcb_setup_t* xcb_get_setup(xcb_connection_t *c);
xcb_screen_iterator_t xcb_setup_roots_iterator(const xcb_setup_t *R);
xcb_generic_event_t* xcb_poll_for_event(xcb_connection_t *c);
int xcb_poll_for_reply(
		xcb_connection_t *c,
		unsigned int request,
		void **reply,
		xcb_generic_error_t **error);
unsigned int xcb_send_request(
		xcb_connection_t *c,
		int flags,
		struct iovec *vector,
		const xcb_protocol_request_t *request);
void *xcb_wait_for_reply(
		xcb_connection_t *c,
		unsigned int request,
		xcb_generic_error_t **e);
xcb_generic_error_t* xcb_request_check(
		xcb_connection_t *c,
		xcb_void_cookie_t cookie);
const xcb_query_extension_reply_t* xcb_get_extension_data(
		xcb_connection_t *c,
		xcb_extension_t *ext);
xcb_intern_atom_cookie_t xcb_intern_atom(
		xcb_connection_t *c,
		uint8_t only_if_exists,
		uint16_t name_len,
		const char *name);
xcb_intern_atom_reply_t* xcb_intern_atom_reply(
		xcb_connection_t *c,
		xcb_intern_atom_cookie_t cookie,
		xcb_generic_error_t **e);
xcb_get_property_cookie_t xcb_get_property(
		xcb_connection_t *c,
		uint8_t _delete,
		xcb_window_t window,
		xcb_atom_t property,
		xcb_atom_t type,
		uint32_t long_offset,
		uint32_t long_length);
xcb_get_property_reply_t* xcb_get_property_reply(
		xcb_connection_t *c,
		xcb_get_property_cookie_t cookie,
		xcb_generic_error_t **e);
void* xcb_get_property_value(const xcb_get_property_reply_t *R);
int xcb_get_property_value_length(
		const xcb_get_property_reply_t *R);
xcb_get_window_attributes_cookie_t xcb_get_window_attributes(
		xcb_connection_t *c,
		xcb_window_t window);
xcb_get_window_attributes_reply_t* xcb_get_window_attributes_reply(
		xcb_connection_t *c,
		xcb_get_window_attributes_cookie_t cookie,
		xcb_generic_error_t **e);
xcb_get_input_focus_cookie_t xcb_get_input_focus(
		xcb_connection_t *c);
xcb_get_input_focus_reply_t* xcb_get_input_focus_reply(
		xcb_connection_t *c,
		xcb_get_input_focus_cookie_t cookie,
		xcb_generic_error_t **e);
xcb_get_selection_owner_cookie_t xcb_get_selection_owner(
		xcb_connection_t *c,
		xcb_atom_t selection);
xcb_get_selection_owner_reply_t* xcb_get_selection_owner_reply(
		xcb_connection_t *c,
		xcb_get_selection_owner_cookie_t cookie,
		xcb_generic_error_t **e);
xcb_query_extension_cookie_t xcb_query_extension(
		xcb_connection_t *c,
		uint16_t name_len,
		const char *name);
xcb_query_extension_reply_t* xcb_query_extension_reply(
		xcb_connection_t *c,
		xcb_query_extension_cookie_t cookie,
		xcb_generic_error_t **e);
xcb_void_cookie_t xcb_change_property_checked(
		xcb_connection_t *c,
		uint8_t mode,
		xcb_window_t window,
		xcb_atom_t property,
		xcb_atom_t type,
		uint8_t format,
		uint32_t data_len,
		const void *data);
xcb_void_cookie_t xcb_change_window_attributes_checked(
		xcb_connection_t *c,
		xcb_window_t window,
		uint32_t value_mask,
		const void *value_list);
xcb_void_cookie_t xcb_send_event_checked(
		xcb_connection_t *c,
		uint8_t propagate,
		xcb_window_t destination,
		uint32_t event_mask,
		const char *event);
xcb_void_cookie_t xcb_map_window_checked(
		xcb_connection_t *c,
		xcb_window_t window);
xcb_void_cookie_t xcb_configure_window_checked(
		xcb_connection_t *c,
		xcb_window_t window,
		uint16_t value_mask,
		const void *value_list);
xcb_void_cookie_t xcb_force_screen_saver_checked(
		xcb_connection_t *c,
		uint8_t mode);

} // namespace base::Platform::XCB::Library
