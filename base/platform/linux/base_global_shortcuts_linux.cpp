// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_global_shortcuts_linux.h"

#include "base/const_string.h"
#include "base/global_shortcuts_generic.h"
#include "base/platform/base_platform_info.h" // IsX11
#include "base/debug_log.h"

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
#include "base/platform/linux/base_linux_xcb_utilities.h" // CustomConnection, IsExtensionPresent
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

#include <QKeySequence>
#include <QSocketNotifier>

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
#include <xcb/record.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h> // xcb_key_symbols_*
#include <xcb/xcbext.h> // xcb_poll_for_reply

#include <xkbcommon/xkbcommon-keysyms.h>
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

class QKeyEvent;

namespace base::Platform::GlobalShortcuts {
namespace {

constexpr auto kShiftMouseButton = std::numeric_limits<uint64>::max() - 100;

Fn<void(GlobalShortcutKeyGeneric descriptor, bool down)> ProcessCallback;

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
using XcbReply = xcb_record_enable_context_reply_t;

bool IsKeypad(xcb_keysym_t keysym) {
	return (xcb_is_keypad_key(keysym) || xcb_is_private_keypad_key(keysym));
}

bool SkipMouseButton(xcb_button_t b) {
	return (b == 1) // Ignore the left button.
		|| (b > 3 && b < 8); // Ignore the wheel.
}

class X11Manager final {
public:
	X11Manager();
	~X11Manager();

	[[nodiscard]] bool available() const;

private:
	void process(XcbReply *reply);
	xcb_keysym_t computeKeysym(xcb_keycode_t detail, uint16_t state);

	XCB::CustomConnection _connection;
	std::unique_ptr<
		xcb_key_symbols_t,
		custom_delete<xcb_key_symbols_free>
	> _keySymbols;
	std::unique_ptr<QSocketNotifier> _notifier;
	std::optional<xcb_record_context_t> _context;
	std::optional<xcb_record_enable_context_cookie_t> _cookie;

};

X11Manager::X11Manager()
: _keySymbols(xcb_key_symbols_alloc(_connection)) {

	if (xcb_connection_has_error(_connection)) {
		LOG((
			"Global Shortcuts Manager: Error to open local display!"));
		return;
	}

	if (!XCB::IsExtensionPresent(_connection, &xcb_record_id)) {
		LOG(("Global Shortcuts Manager: "
			"RECORD extension not supported on this X server!"));
		return;
	}

	_context = xcb_generate_id(_connection);
	const xcb_record_client_spec_t clientSpec[] = {
		XCB_RECORD_CS_ALL_CLIENTS
	};

	const xcb_record_range_t recordRange[] = {
		[] {
			xcb_record_range_t rr;
			memset(&rr, 0, sizeof(rr));

			// XCB_KEY_PRESS = 2
			// XCB_KEY_RELEASE = 3
			// XCB_BUTTON_PRESS = 4
			// XCB_BUTTON_RELEASE = 5
			rr.device_events = { XCB_KEY_PRESS, XCB_BUTTON_RELEASE };
			return rr;
		}()
	};

	const auto createCookie = xcb_record_create_context_checked(
		_connection,
		*_context,
		0,
		sizeof(clientSpec) / sizeof(clientSpec[0]),
		sizeof(recordRange) / sizeof(recordRange[0]),
		clientSpec,
		recordRange);
	if (const auto error = xcb_request_check(_connection, createCookie)) {
		LOG((
			"Global Shortcuts Manager: Could not create a record context!"));
		_context = std::nullopt;
		free(error);
		return;
	}

	_cookie = xcb_record_enable_context(_connection, *_context);
	xcb_flush(_connection);

	_notifier = std::make_unique<QSocketNotifier>(
		xcb_get_file_descriptor(_connection),
		QSocketNotifier::Read);

	QObject::connect(_notifier.get(), &QSocketNotifier::activated, [=] {
		while (const auto event = xcb_poll_for_event(_connection)) {
			free(event);
		}

		void *reply = nullptr;
		xcb_generic_error_t *error = nullptr;
		while (_cookie
			&& _cookie->sequence
			&& xcb_poll_for_reply(
				_connection,
				_cookie->sequence,
				&reply,
				&error)) {
			// The xcb_poll_for_reply method may set both reply and error
			// to null if connection has error.
			if (xcb_connection_has_error(_connection)) {
				break;
			}

			if (error) {
				free(error);
				break;
			}

			if (!reply) {
				continue;
			}

			process(reinterpret_cast<XcbReply*>(reply));
			free(reply);
		}
	});
	_notifier->setEnabled(true);
}


X11Manager::~X11Manager() {
	if (_cookie) {
		xcb_record_disable_context(_connection, *_context);
		_cookie = std::nullopt;
	}

	if (_context) {
		xcb_record_free_context(_connection, *_context);
		_context = std::nullopt;
	}
}

void X11Manager::process(XcbReply *reply) {
	if (!ProcessCallback) {
		return;
	}
	// Seems like xcb_button_press_event_t and xcb_key_press_event_t structs
	// are the same, so we can safely cast both of them
	// to the xcb_key_press_event_t.
	const auto events = reinterpret_cast<xcb_key_press_event_t*>(
		xcb_record_enable_context_data(reply));

	const auto countEvents = xcb_record_enable_context_data_length(reply) /
		sizeof(xcb_key_press_event_t);

	for (auto e = events; e < (events + countEvents); e++) {
		const auto type = e->response_type;
		const auto buttonPress = (type == XCB_BUTTON_PRESS);
		const auto buttonRelease = (type == XCB_BUTTON_RELEASE);
		const auto keyPress = (type == XCB_KEY_PRESS);
		const auto keyRelease = (type == XCB_KEY_RELEASE);
		const auto isButton = (buttonPress || buttonRelease);

		if (!(keyPress || keyRelease || isButton)) {
			continue;
		}
		const auto code = e->detail;
		if (isButton && SkipMouseButton(code)) {
			return;
		}
		const auto descriptor = isButton
			? (kShiftMouseButton + code)
			: GlobalShortcutKeyGeneric(computeKeysym(code, e->state));
		ProcessCallback(descriptor, keyPress || buttonPress);
	}
}

xcb_keysym_t X11Manager::computeKeysym(xcb_keycode_t detail, uint16_t state) {
	// Perhaps XCB_MOD_MASK_1-5 are needed here.
	const auto keySym1 = xcb_key_symbols_get_keysym(_keySymbols.get(), detail, 1);
	if (IsKeypad(keySym1)) {
		return keySym1;
	}
	if (keySym1 >= Qt::Key_A && keySym1 <= Qt::Key_Z) {
		if (keySym1 != XCB_NO_SYMBOL) {
			return keySym1;
		}
	}

	return xcb_key_symbols_get_keysym(_keySymbols.get(), detail, 0);
}

bool X11Manager::available() const {
	return _cookie.has_value();
}

std::unique_ptr<X11Manager> _x11Manager = nullptr;

void EnsureX11ShortcutManager() {
	if (!_x11Manager) {
		_x11Manager = std::make_unique<X11Manager>();
	}
}
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

} // namespace

bool Available() {
#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
	if (::Platform::IsX11()) {
		EnsureX11ShortcutManager();
		return _x11Manager->available();
	}
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

	return false;
}

bool Allowed() {
	return Available();
}

void Start(Fn<void(GlobalShortcutKeyGeneric descriptor, bool down)> process) {
	ProcessCallback = std::move(process);

#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
	EnsureX11ShortcutManager();
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION
}

void Stop() {
	ProcessCallback = nullptr;
#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
	_x11Manager = nullptr;
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION
}

QString KeyName(GlobalShortcutKeyGeneric descriptor) {
#ifndef DESKTOP_APP_DISABLE_X11_INTEGRATION
	// Telegram/ThirdParty/fcitx-qt5/platforminputcontext/qtkey.cpp
	static const auto KeyToString = flat_map<uint64, int>{
		{ XKB_KEY_KP_Space, Qt::Key_Space },
		{ XKB_KEY_KP_Tab, Qt::Key_Tab },
		{ XKB_KEY_KP_Enter, Qt::Key_Enter },
		{ XKB_KEY_KP_F1, Qt::Key_F1 },
		{ XKB_KEY_KP_F2, Qt::Key_F2 },
		{ XKB_KEY_KP_F3, Qt::Key_F3 },
		{ XKB_KEY_KP_F4, Qt::Key_F4 },
		{ XKB_KEY_KP_Home, Qt::Key_Home },
		{ XKB_KEY_KP_Left, Qt::Key_Left },
		{ XKB_KEY_KP_Up, Qt::Key_Up },
		{ XKB_KEY_KP_Right, Qt::Key_Right },
		{ XKB_KEY_KP_Down, Qt::Key_Down },
		{ XKB_KEY_KP_Page_Up, Qt::Key_PageUp },
		{ XKB_KEY_KP_Page_Down, Qt::Key_PageDown },
		{ XKB_KEY_KP_End, Qt::Key_End },
		{ XKB_KEY_KP_Begin, Qt::Key_Clear },
		{ XKB_KEY_KP_Insert, Qt::Key_Insert },
		{ XKB_KEY_KP_Delete, Qt::Key_Delete },
		{ XKB_KEY_KP_Equal, Qt::Key_Equal },
		{ XKB_KEY_KP_Multiply, Qt::Key_multiply },
		{ XKB_KEY_KP_Add, Qt::Key_Plus },
		{ XKB_KEY_KP_Separator, Qt::Key_Comma },
		{ XKB_KEY_KP_Subtract, Qt::Key_Minus },
		{ XKB_KEY_KP_Decimal, Qt::Key_Period },
		{ XKB_KEY_KP_Divide, Qt::Key_Slash },

		{ XKB_KEY_KP_0, Qt::Key_0 },
		{ XKB_KEY_KP_1, Qt::Key_1 },
		{ XKB_KEY_KP_2, Qt::Key_2 },
		{ XKB_KEY_KP_3, Qt::Key_3 },
		{ XKB_KEY_KP_4, Qt::Key_4 },
		{ XKB_KEY_KP_5, Qt::Key_5 },
		{ XKB_KEY_KP_6, Qt::Key_6 },
		{ XKB_KEY_KP_7, Qt::Key_7 },
		{ XKB_KEY_KP_8, Qt::Key_8 },
		{ XKB_KEY_KP_9, Qt::Key_9 },

		{ XKB_KEY_Escape, Qt::Key_Escape },
		{ XKB_KEY_Tab, Qt::Key_Tab },
		{ XKB_KEY_ISO_Left_Tab, Qt::Key_Tab },
		{ XKB_KEY_BackSpace, Qt::Key_Backspace },
		{ XKB_KEY_Return, Qt::Key_Return },
		{ XKB_KEY_KP_Enter, Qt::Key_Enter },
		{ XKB_KEY_Insert, Qt::Key_Insert },
		{ XKB_KEY_Delete, Qt::Key_Delete },
		{ XKB_KEY_Clear, Qt::Key_Delete },
		{ XKB_KEY_Pause, Qt::Key_Pause },
		{ XKB_KEY_Print, Qt::Key_Print },
		{ XKB_KEY_Sys_Req, Qt::Key_SysReq },
		{ XKB_KEY_SunSys_Req, Qt::Key_SysReq },
		{ 0x1007ff00, Qt::Key_SysReq },

		{ XKB_KEY_Home, Qt::Key_Home },
		{ XKB_KEY_End, Qt::Key_End },
		{ XKB_KEY_Left, Qt::Key_Left },
		{ XKB_KEY_Up, Qt::Key_Up },
		{ XKB_KEY_Right, Qt::Key_Right },
		{ XKB_KEY_Down, Qt::Key_Down },
		{ XKB_KEY_Page_Up, Qt::Key_PageUp },
		{ XKB_KEY_Page_Down, Qt::Key_PageDown },
		{ XKB_KEY_Shift_L, Qt::Key_Shift },
		{ XKB_KEY_Shift_R, Qt::Key_Shift },
		{ XKB_KEY_Shift_Lock, Qt::Key_Shift },
		{ XKB_KEY_Control_L, Qt::Key_Control },
		{ XKB_KEY_Control_R, Qt::Key_Control },
		{ XKB_KEY_Meta_L, Qt::Key_Meta },
		{ XKB_KEY_Meta_R, Qt::Key_Meta },
		{ XKB_KEY_Alt_L, Qt::Key_Alt },
		{ XKB_KEY_Alt_R, Qt::Key_Alt },
		{ XKB_KEY_Caps_Lock, Qt::Key_CapsLock },
		{ XKB_KEY_Num_Lock, Qt::Key_NumLock },
		{ XKB_KEY_Scroll_Lock, Qt::Key_ScrollLock },
		{ XKB_KEY_F1, Qt::Key_F1 },
		{ XKB_KEY_F2, Qt::Key_F2 },
		{ XKB_KEY_F3, Qt::Key_F3 },
		{ XKB_KEY_F4, Qt::Key_F4 },
		{ XKB_KEY_F5, Qt::Key_F5 },
		{ XKB_KEY_F6, Qt::Key_F6 },
		{ XKB_KEY_F7, Qt::Key_F7 },
		{ XKB_KEY_F8, Qt::Key_F8 },
		{ XKB_KEY_F9, Qt::Key_F9 },
		{ XKB_KEY_F10, Qt::Key_F10 },
		{ XKB_KEY_F11, Qt::Key_F11 },
		{ XKB_KEY_F12, Qt::Key_F12 },
		{ XKB_KEY_F13, Qt::Key_F13 },
		{ XKB_KEY_F14, Qt::Key_F14 },
		{ XKB_KEY_F15, Qt::Key_F15 },
		{ XKB_KEY_F16, Qt::Key_F16 },
		{ XKB_KEY_F17, Qt::Key_F17 },
		{ XKB_KEY_F18, Qt::Key_F18 },
		{ XKB_KEY_F19, Qt::Key_F19 },
		{ XKB_KEY_F20, Qt::Key_F20 },
		{ XKB_KEY_F21, Qt::Key_F21 },
		{ XKB_KEY_F22, Qt::Key_F22 },
		{ XKB_KEY_F23, Qt::Key_F23 },
		{ XKB_KEY_F24, Qt::Key_F24 },
		{ XKB_KEY_F25, Qt::Key_F25 },
		{ XKB_KEY_F26, Qt::Key_F26 },
		{ XKB_KEY_F27, Qt::Key_F27 },
		{ XKB_KEY_F28, Qt::Key_F28 },
		{ XKB_KEY_F29, Qt::Key_F29 },
		{ XKB_KEY_F30, Qt::Key_F30 },
		{ XKB_KEY_F31, Qt::Key_F31 },
		{ XKB_KEY_F32, Qt::Key_F32 },
		{ XKB_KEY_F33, Qt::Key_F33 },
		{ XKB_KEY_F34, Qt::Key_F34 },
		{ XKB_KEY_F35, Qt::Key_F35 },
		{ XKB_KEY_Super_L, Qt::Key_Super_L },
		{ XKB_KEY_Super_R, Qt::Key_Super_R },
		{ XKB_KEY_Menu, Qt::Key_Menu },
		{ XKB_KEY_Hyper_L, Qt::Key_Hyper_L },
		{ XKB_KEY_Hyper_R, Qt::Key_Hyper_R },
		{ XKB_KEY_Help, Qt::Key_Help },
		{ XKB_KEY_ISO_Level3_Shift, Qt::Key_AltGr },
		{ XKB_KEY_Multi_key, Qt::Key_Multi_key },
		{ XKB_KEY_Codeinput, Qt::Key_Codeinput },
		{ XKB_KEY_SingleCandidate, Qt::Key_SingleCandidate },
		{ XKB_KEY_MultipleCandidate, Qt::Key_MultipleCandidate },
		{ XKB_KEY_PreviousCandidate, Qt::Key_PreviousCandidate },
		{ XKB_KEY_Mode_switch, Qt::Key_Mode_switch },
		{ XKB_KEY_script_switch, Qt::Key_Mode_switch },
		{ XKB_KEY_Kanji, Qt::Key_Kanji },
		{ XKB_KEY_Muhenkan, Qt::Key_Muhenkan },
		{ XKB_KEY_Henkan, Qt::Key_Henkan },
		{ XKB_KEY_Romaji, Qt::Key_Romaji },
		{ XKB_KEY_Hiragana, Qt::Key_Hiragana },
		{ XKB_KEY_Katakana, Qt::Key_Katakana },
		{ XKB_KEY_Hiragana_Katakana, Qt::Key_Hiragana_Katakana },
		{ XKB_KEY_Zenkaku, Qt::Key_Zenkaku },
		{ XKB_KEY_Hankaku, Qt::Key_Hankaku },
		{ XKB_KEY_Zenkaku_Hankaku, Qt::Key_Zenkaku_Hankaku },
		{ XKB_KEY_Touroku, Qt::Key_Touroku },
		{ XKB_KEY_Massyo, Qt::Key_Massyo },
		{ XKB_KEY_Kana_Lock, Qt::Key_Kana_Lock },
		{ XKB_KEY_Kana_Shift, Qt::Key_Kana_Shift },
		{ XKB_KEY_Eisu_Shift, Qt::Key_Eisu_Shift },
		{ XKB_KEY_Eisu_toggle, Qt::Key_Eisu_toggle },
		{ XKB_KEY_Kanji_Bangou, Qt::Key_Codeinput },
		{ XKB_KEY_Zen_Koho, Qt::Key_MultipleCandidate },
		{ XKB_KEY_Mae_Koho, Qt::Key_PreviousCandidate },
		{ XKB_KEY_Hangul, Qt::Key_Hangul },
		{ XKB_KEY_Hangul_Start, Qt::Key_Hangul_Start },
		{ XKB_KEY_Hangul_End, Qt::Key_Hangul_End },
		{ XKB_KEY_Hangul_Hanja, Qt::Key_Hangul_Hanja },
		{ XKB_KEY_Hangul_Jamo, Qt::Key_Hangul_Jamo },
		{ XKB_KEY_Hangul_Romaja, Qt::Key_Hangul_Romaja },
		{ XKB_KEY_Hangul_Codeinput, Qt::Key_Codeinput },
		{ XKB_KEY_Hangul_Jeonja, Qt::Key_Hangul_Jeonja },
		{ XKB_KEY_Hangul_Banja, Qt::Key_Hangul_Banja },
		{ XKB_KEY_Hangul_PreHanja, Qt::Key_Hangul_PreHanja },
		{ XKB_KEY_Hangul_PostHanja, Qt::Key_Hangul_PostHanja },
		{ XKB_KEY_Hangul_SingleCandidate, Qt::Key_SingleCandidate },
		{ XKB_KEY_Hangul_MultipleCandidate, Qt::Key_MultipleCandidate },
		{ XKB_KEY_Hangul_PreviousCandidate, Qt::Key_PreviousCandidate },
		{ XKB_KEY_Hangul_Special, Qt::Key_Hangul_Special },
		{ XKB_KEY_Hangul_switch, Qt::Key_Mode_switch },
		{ XKB_KEY_dead_grave, Qt::Key_Dead_Grave },
		{ XKB_KEY_dead_acute, Qt::Key_Dead_Acute },
		{ XKB_KEY_dead_circumflex, Qt::Key_Dead_Circumflex },
		{ XKB_KEY_dead_tilde, Qt::Key_Dead_Tilde },
		{ XKB_KEY_dead_macron, Qt::Key_Dead_Macron },
		{ XKB_KEY_dead_breve, Qt::Key_Dead_Breve },
		{ XKB_KEY_dead_abovedot, Qt::Key_Dead_Abovedot },
		{ XKB_KEY_dead_diaeresis, Qt::Key_Dead_Diaeresis },
		{ XKB_KEY_dead_abovering, Qt::Key_Dead_Abovering },
		{ XKB_KEY_dead_doubleacute, Qt::Key_Dead_Doubleacute },
		{ XKB_KEY_dead_caron, Qt::Key_Dead_Caron },
		{ XKB_KEY_dead_cedilla, Qt::Key_Dead_Cedilla },
		{ XKB_KEY_dead_ogonek, Qt::Key_Dead_Ogonek },
		{ XKB_KEY_dead_iota, Qt::Key_Dead_Iota },
		{ XKB_KEY_dead_voiced_sound, Qt::Key_Dead_Voiced_Sound },
		{ XKB_KEY_dead_semivoiced_sound, Qt::Key_Dead_Semivoiced_Sound },
		{ XKB_KEY_dead_belowdot, Qt::Key_Dead_Belowdot },
		{ XKB_KEY_dead_hook, Qt::Key_Dead_Hook },
		{ XKB_KEY_dead_horn, Qt::Key_Dead_Horn },
		{ XKB_KEY_XF86Back, Qt::Key_Back },
		{ XKB_KEY_XF86Forward, Qt::Key_Forward },
		{ XKB_KEY_XF86Stop, Qt::Key_Stop },
		{ XKB_KEY_XF86Refresh, Qt::Key_Refresh },
		{ XKB_KEY_XF86AudioLowerVolume, Qt::Key_VolumeDown },
		{ XKB_KEY_XF86AudioMute, Qt::Key_VolumeMute },
		{ XKB_KEY_XF86AudioRaiseVolume, Qt::Key_VolumeUp },
		{ XKB_KEY_XF86AudioPlay, Qt::Key_MediaPlay },
		{ XKB_KEY_XF86AudioStop, Qt::Key_MediaStop },
		{ XKB_KEY_XF86AudioPrev, Qt::Key_MediaPrevious },
		{ XKB_KEY_XF86AudioNext, Qt::Key_MediaNext },
		{ XKB_KEY_XF86AudioRecord, Qt::Key_MediaRecord },
		{ XKB_KEY_XF86AudioPause, Qt::Key_MediaPause },
		{ XKB_KEY_XF86HomePage, Qt::Key_HomePage },
		{ XKB_KEY_XF86Favorites, Qt::Key_Favorites },
		{ XKB_KEY_XF86Search, Qt::Key_Search },
		{ XKB_KEY_XF86Standby, Qt::Key_Standby },
		{ XKB_KEY_XF86OpenURL, Qt::Key_OpenUrl },
		{ XKB_KEY_XF86Mail, Qt::Key_LaunchMail },
		{ XKB_KEY_XF86AudioMedia, Qt::Key_LaunchMedia },
		{ XKB_KEY_XF86MyComputer, Qt::Key_Launch0 },
		{ XKB_KEY_XF86Calculator, Qt::Key_Launch1 },
		{ XKB_KEY_XF86Launch0, Qt::Key_Launch2 },
		{ XKB_KEY_XF86Launch1, Qt::Key_Launch3 },
		{ XKB_KEY_XF86Launch2, Qt::Key_Launch4 },
		{ XKB_KEY_XF86Launch3, Qt::Key_Launch5 },
		{ XKB_KEY_XF86Launch4, Qt::Key_Launch6 },
		{ XKB_KEY_XF86Launch5, Qt::Key_Launch7 },
		{ XKB_KEY_XF86Launch6, Qt::Key_Launch8 },
		{ XKB_KEY_XF86Launch7, Qt::Key_Launch9 },
		{ XKB_KEY_XF86Launch8, Qt::Key_LaunchA },
		{ XKB_KEY_XF86Launch9, Qt::Key_LaunchB },
		{ XKB_KEY_XF86LaunchA, Qt::Key_LaunchC },
		{ XKB_KEY_XF86LaunchB, Qt::Key_LaunchD },
		{ XKB_KEY_XF86LaunchC, Qt::Key_LaunchE },
		{ XKB_KEY_XF86LaunchD, Qt::Key_LaunchF },
		{ XKB_KEY_XF86MonBrightnessUp, Qt::Key_MonBrightnessUp },
		{ XKB_KEY_XF86MonBrightnessDown, Qt::Key_MonBrightnessDown },
		{ XKB_KEY_XF86KbdLightOnOff, Qt::Key_KeyboardLightOnOff },
		{ XKB_KEY_XF86KbdBrightnessUp, Qt::Key_KeyboardBrightnessUp },
		{ XKB_KEY_XF86PowerOff, Qt::Key_PowerOff },
		{ XKB_KEY_XF86WakeUp, Qt::Key_WakeUp },
		{ XKB_KEY_XF86Eject, Qt::Key_Eject },
		{ XKB_KEY_XF86ScreenSaver, Qt::Key_ScreenSaver },
		{ XKB_KEY_XF86WWW, Qt::Key_WWW },
		{ XKB_KEY_XF86Memo, Qt::Key_Memo },
		{ XKB_KEY_XF86LightBulb, Qt::Key_LightBulb },
		{ XKB_KEY_XF86Shop, Qt::Key_Shop },
		{ XKB_KEY_XF86History, Qt::Key_History },
		{ XKB_KEY_XF86AddFavorite, Qt::Key_AddFavorite },
		{ XKB_KEY_XF86HotLinks, Qt::Key_HotLinks },
		{ XKB_KEY_XF86BrightnessAdjust, Qt::Key_BrightnessAdjust },
		{ XKB_KEY_XF86Finance, Qt::Key_Finance },
		{ XKB_KEY_XF86Community, Qt::Key_Community },
		{ XKB_KEY_XF86AudioRewind, Qt::Key_AudioRewind },
		{ XKB_KEY_XF86BackForward, Qt::Key_BackForward },
		{ XKB_KEY_XF86ApplicationLeft, Qt::Key_ApplicationLeft },
		{ XKB_KEY_XF86ApplicationRight, Qt::Key_ApplicationRight },
		{ XKB_KEY_XF86Book, Qt::Key_Book },
		{ XKB_KEY_XF86CD, Qt::Key_CD },
		{ XKB_KEY_XF86Calculater, Qt::Key_Calculator },
		{ XKB_KEY_XF86ToDoList, Qt::Key_ToDoList },
		{ XKB_KEY_XF86ClearGrab, Qt::Key_ClearGrab },
		{ XKB_KEY_XF86Close, Qt::Key_Close },
		{ XKB_KEY_XF86Copy, Qt::Key_Copy },
		{ XKB_KEY_XF86Cut, Qt::Key_Cut },
		{ XKB_KEY_XF86Display, Qt::Key_Display },
		{ XKB_KEY_XF86DOS, Qt::Key_DOS },
		{ XKB_KEY_XF86Documents, Qt::Key_Documents },
		{ XKB_KEY_XF86Excel, Qt::Key_Excel },
		{ XKB_KEY_XF86Explorer, Qt::Key_Explorer },
		{ XKB_KEY_XF86Game, Qt::Key_Game },
		{ XKB_KEY_XF86Go, Qt::Key_Go },
		{ XKB_KEY_XF86iTouch, Qt::Key_iTouch },
		{ XKB_KEY_XF86LogOff, Qt::Key_LogOff },
		{ XKB_KEY_XF86Market, Qt::Key_Market },
		{ XKB_KEY_XF86Meeting, Qt::Key_Meeting },
		{ XKB_KEY_XF86MenuKB, Qt::Key_MenuKB },
		{ XKB_KEY_XF86MenuPB, Qt::Key_MenuPB },
		{ XKB_KEY_XF86MySites, Qt::Key_MySites },
		{ XKB_KEY_XF86News, Qt::Key_News },
		{ XKB_KEY_XF86OfficeHome, Qt::Key_OfficeHome },
		{ XKB_KEY_XF86Option, Qt::Key_Option },
		{ XKB_KEY_XF86Paste, Qt::Key_Paste },
		{ XKB_KEY_XF86Phone, Qt::Key_Phone },
		{ XKB_KEY_XF86Calendar, Qt::Key_Calendar },
		{ XKB_KEY_XF86Reply, Qt::Key_Reply },
		{ XKB_KEY_XF86Reload, Qt::Key_Reload },
		{ XKB_KEY_XF86RotateWindows, Qt::Key_RotateWindows },
		{ XKB_KEY_XF86RotationPB, Qt::Key_RotationPB },
		{ XKB_KEY_XF86RotationKB, Qt::Key_RotationKB },
		{ XKB_KEY_XF86Save, Qt::Key_Save },
		{ XKB_KEY_XF86Send, Qt::Key_Send },
		{ XKB_KEY_XF86Spell, Qt::Key_Spell },
		{ XKB_KEY_XF86SplitScreen, Qt::Key_SplitScreen },
		{ XKB_KEY_XF86Support, Qt::Key_Support },
		{ XKB_KEY_XF86TaskPane, Qt::Key_TaskPane },
		{ XKB_KEY_XF86Terminal, Qt::Key_Terminal },
		{ XKB_KEY_XF86Tools, Qt::Key_Tools },
		{ XKB_KEY_XF86Travel, Qt::Key_Travel },
		{ XKB_KEY_XF86Video, Qt::Key_Video },
		{ XKB_KEY_XF86Word, Qt::Key_Word },
		{ XKB_KEY_XF86Xfer, Qt::Key_Xfer },
		{ XKB_KEY_XF86ZoomIn, Qt::Key_ZoomIn },
		{ XKB_KEY_XF86ZoomOut, Qt::Key_ZoomOut },
		{ XKB_KEY_XF86Away, Qt::Key_Away },
		{ XKB_KEY_XF86Messenger, Qt::Key_Messenger },
		{ XKB_KEY_XF86WebCam, Qt::Key_WebCam },
		{ XKB_KEY_XF86MailForward, Qt::Key_MailForward },
		{ XKB_KEY_XF86Pictures, Qt::Key_Pictures },
		{ XKB_KEY_XF86Music, Qt::Key_Music },
		{ XKB_KEY_XF86Battery, Qt::Key_Battery },
		{ XKB_KEY_XF86Bluetooth, Qt::Key_Bluetooth },
		{ XKB_KEY_XF86WLAN, Qt::Key_WLAN },
		{ XKB_KEY_XF86UWB, Qt::Key_UWB },
		{ XKB_KEY_XF86AudioForward, Qt::Key_AudioForward },
		{ XKB_KEY_XF86AudioRepeat, Qt::Key_AudioRepeat },
		{ XKB_KEY_XF86AudioRandomPlay, Qt::Key_AudioRandomPlay },
		{ XKB_KEY_XF86Subtitle, Qt::Key_Subtitle },
		{ XKB_KEY_XF86AudioCycleTrack, Qt::Key_AudioCycleTrack },
		{ XKB_KEY_XF86Time, Qt::Key_Time },
		{ XKB_KEY_XF86Hibernate, Qt::Key_Hibernate },
		{ XKB_KEY_XF86View, Qt::Key_View },
		{ XKB_KEY_XF86TopMenu, Qt::Key_TopMenu },
		{ XKB_KEY_XF86PowerDown, Qt::Key_PowerDown },
		{ XKB_KEY_XF86Suspend, Qt::Key_Suspend },
		{ XKB_KEY_XF86ContrastAdjust, Qt::Key_ContrastAdjust },

		{ XKB_KEY_XF86LaunchE, Qt::Key_LaunchG },
		{ XKB_KEY_XF86LaunchF, Qt::Key_LaunchH },

		{ XKB_KEY_XF86Select, Qt::Key_Select },
		{ XKB_KEY_Cancel, Qt::Key_Cancel },
		{ XKB_KEY_Execute, Qt::Key_Execute },
		{ XKB_KEY_XF86Sleep, Qt::Key_Sleep },
	};

	// Mouse.
	// Taken from QXcbConnection::translateMouseButton.
	static const auto XcbButtonToQt = flat_map<uint64, Qt::MouseButton>{
		// { 1, Qt::LeftButton }, // Ignore the left button.
		{ 2, Qt::MiddleButton },
		{ 3, Qt::RightButton },
		// Button values 4-7 were already handled as Wheel events.
		{ 8, Qt::BackButton },
		{ 9, Qt::ForwardButton },
		{ 10, Qt::ExtraButton3 },
		{ 11, Qt::ExtraButton4 },
		{ 12, Qt::ExtraButton5 },
		{ 13, Qt::ExtraButton6 },
		{ 14, Qt::ExtraButton7 },
		{ 15, Qt::ExtraButton8 },
		{ 16, Qt::ExtraButton9 },
		{ 17, Qt::ExtraButton10 },
		{ 18, Qt::ExtraButton11 },
		{ 19, Qt::ExtraButton12 },
		{ 20, Qt::ExtraButton13 },
		{ 21, Qt::ExtraButton14 },
		{ 22, Qt::ExtraButton15 },
		{ 23, Qt::ExtraButton16 },
		{ 24, Qt::ExtraButton17 },
		{ 25, Qt::ExtraButton18 },
		{ 26, Qt::ExtraButton19 },
		{ 27, Qt::ExtraButton20 },
		{ 28, Qt::ExtraButton21 },
		{ 29, Qt::ExtraButton22 },
		{ 30, Qt::ExtraButton23 },
		{ 31, Qt::ExtraButton24 },
	};
	if (descriptor > kShiftMouseButton) {
		const auto button = descriptor - kShiftMouseButton;
		if (XcbButtonToQt.contains(button)) {
			return QString("Mouse %1").arg(button);
		}
	}
	//

	// Modifiers.
	static const auto ModifierToString = flat_map<uint64, const_string>{
		{ XKB_KEY_Shift_L, "Shift" },
		{ XKB_KEY_Shift_R, "Right Shift" },
		{ XKB_KEY_Control_L, "Ctrl" },
		{ XKB_KEY_Control_R, "Right Ctrl" },
		{ XKB_KEY_Meta_L, "Meta" },
		{ XKB_KEY_Meta_R, "Right Meta" },
		{ XKB_KEY_Alt_L, "Alt" },
		{ XKB_KEY_Alt_R, "Right Alt" },
		{ XKB_KEY_Super_L, "Super" },
		{ XKB_KEY_Super_R, "Right Super" },
	};
	const auto modIt = ModifierToString.find(descriptor);
	if (modIt != end(ModifierToString)) {
		return modIt->second.utf16();
	}
	//

	const auto fromSequence = [](int k) {
		return QKeySequence(k).toString(QKeySequence::NativeText);
	};

	// The conversion is not necessary,
	// if the value in the range Qt::Key_Space - Qt::Key_QuoteLeft.
	if (descriptor >= Qt::Key_Space && descriptor <= Qt::Key_QuoteLeft) {
		return fromSequence(descriptor);
	}
	const auto prefix = IsKeypad(descriptor) ? "Num " : QString();

	const auto keyIt = KeyToString.find(descriptor);
	return (keyIt != end(KeyToString))
		? prefix + fromSequence(keyIt->second)
		: QString("\\x%1").arg(descriptor, 0, 16);
#endif // !DESKTOP_APP_DISABLE_X11_INTEGRATION

	return {};
}

bool IsToggleFullScreenKey(not_null<QKeyEvent*> e) {
	return false;
}

} // namespace base::Platform::GlobalShortcuts
