// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/mac/base_mac_key_codes.h"

#include "base/const_string.h"

#include <Carbon/Carbon.h>
#import <Foundation/Foundation.h>

namespace base::Platform {
namespace {

const auto KeyToString = flat_map<uint32, const_string>{
	{ kVK_Return, "\xE2\x8F\x8E"_cs },
	{ kVK_Tab, "\xE2\x87\xA5"_cs },
	{ kVK_Space, "\xE2\x90\xA3"_cs },
	{ kVK_Delete, "\xE2\x8C\xAB"_cs },
	{ kVK_Escape, "\xE2\x8E\x8B"_cs },
	{ kVK_Command, "\xE2\x8C\x98"_cs },
	{ kVK_Shift, "\xE2\x87\xA7"_cs },
	{ kVK_CapsLock, "Caps Lock"_cs },
	{ kVK_Option, "\xE2\x8C\xA5"_cs },
	{ kVK_Control, "\xE2\x8C\x83"_cs },
	{ kVK_RightCommand, "Right \xE2\x8C\x98"_cs },
	{ kVK_RightShift, "Right \xE2\x87\xA7"_cs },
	{ kVK_RightOption, "Right \xE2\x8C\xA5"_cs },
	{ kVK_RightControl, "Right \xE2\x8C\x83"_cs },
	{ kVK_Function, "Fn"_cs },
	{ kVK_F17, "F17"_cs },
	{ kVK_VolumeUp, "Volume Up"_cs },
	{ kVK_VolumeDown, "Volume Down"_cs },
	{ kVK_Mute, "Mute"_cs },
	{ kVK_F18, "F18"_cs },
	{ kVK_F19, "F19"_cs },
	{ kVK_F20, "F20"_cs },
	{ kVK_F5, "F5"_cs },
	{ kVK_F6, "F6"_cs },
	{ kVK_F7, "F7"_cs },
	{ kVK_F3, "F3"_cs },
	{ kVK_F8, "F8"_cs },
	{ kVK_F9, "F9"_cs },
	{ kVK_F11, "F11"_cs },
	{ kVK_F13, "F13"_cs },
	{ kVK_F16, "F16"_cs },
	{ kVK_F14, "F14"_cs },
	{ kVK_F10, "F10"_cs },
	{ kVK_F12, "F12"_cs },
	{ kVK_F15, "F15"_cs },
	{ kVK_Help, "Help"_cs },
	{ kVK_Home, "\xE2\x86\x96"_cs },
	{ kVK_PageUp, "Page Up"_cs },
	{ kVK_ForwardDelete, "\xe2\x8c\xa6"_cs },
	{ kVK_F4, "F4"_cs },
	{ kVK_End, "\xE2\x86\x98"_cs },
	{ kVK_F2, "F2"_cs },
	{ kVK_PageDown, "Page Down"_cs },
	{ kVK_F1, "F1"_cs },
	{ kVK_LeftArrow, "\xE2\x86\x90"_cs },
	{ kVK_RightArrow, "\xE2\x86\x92"_cs },
	{ kVK_DownArrow, "\xE2\x86\x93"_cs },
	{ kVK_UpArrow, "\xE2\x86\x91"_cs },

	{ kVK_ANSI_A, "A" },
	{ kVK_ANSI_S, "S" },
	{ kVK_ANSI_D, "D" },
	{ kVK_ANSI_F, "F" },
	{ kVK_ANSI_H, "H" },
	{ kVK_ANSI_G, "G" },
	{ kVK_ANSI_Z, "Z" },
	{ kVK_ANSI_X, "X" },
	{ kVK_ANSI_C, "C" },
	{ kVK_ANSI_V, "V" },
	{ kVK_ANSI_B, "B" },
	{ kVK_ANSI_Q, "Q" },
	{ kVK_ANSI_W, "W" },
	{ kVK_ANSI_E, "E" },
	{ kVK_ANSI_R, "R" },
	{ kVK_ANSI_Y, "Y" },
	{ kVK_ANSI_T, "T" },
	{ kVK_ANSI_1, "1" },
	{ kVK_ANSI_2, "2" },
	{ kVK_ANSI_3, "3" },
	{ kVK_ANSI_4, "4" },
	{ kVK_ANSI_6, "6" },
	{ kVK_ANSI_5, "5" },
	{ kVK_ANSI_Equal, "=" },
	{ kVK_ANSI_9, "9" },
	{ kVK_ANSI_7, "7" },
	{ kVK_ANSI_Minus, "-" },
	{ kVK_ANSI_8, "8" },
	{ kVK_ANSI_0, "0" },
	{ kVK_ANSI_RightBracket, "]" },
	{ kVK_ANSI_O, "O" },
	{ kVK_ANSI_U, "U" },
	{ kVK_ANSI_LeftBracket, "[" },
	{ kVK_ANSI_I, "I" },
	{ kVK_ANSI_P, "P" },
	{ kVK_ANSI_L, "L" },
	{ kVK_ANSI_J, "J" },
	{ kVK_ANSI_Quote, "'" },
	{ kVK_ANSI_K, "K" },
	{ kVK_ANSI_Semicolon, "/" },
	{ kVK_ANSI_Backslash, "\\" },
	{ kVK_ANSI_Comma, "," },
	{ kVK_ANSI_Slash, "/" },
	{ kVK_ANSI_N, "N" },
	{ kVK_ANSI_M, "M" },
	{ kVK_ANSI_Period, "." },
	{ kVK_ANSI_Grave, "`" },
	{ kVK_ANSI_KeypadDecimal, "Num ." },
	{ kVK_ANSI_KeypadMultiply, "Num *" },
	{ kVK_ANSI_KeypadPlus, "Num +" },
	{ kVK_ANSI_KeypadClear, "Num Clear" },
	{ kVK_ANSI_KeypadDivide, "Num /" },
	{ kVK_ANSI_KeypadEnter, "Num Enter" },
	{ kVK_ANSI_KeypadMinus, "Num -" },
	{ kVK_ANSI_KeypadEquals, "Num =" },
	{ kVK_ANSI_Keypad0, "Num 0" },
	{ kVK_ANSI_Keypad1, "Num 1" },
	{ kVK_ANSI_Keypad2, "Num 2" },
	{ kVK_ANSI_Keypad3, "Num 3" },
	{ kVK_ANSI_Keypad4, "Num 4" },
	{ kVK_ANSI_Keypad5, "Num 5" },
	{ kVK_ANSI_Keypad6, "Num 6" },
	{ kVK_ANSI_Keypad7, "Num 7" },
	{ kVK_ANSI_Keypad8, "Num 8" },
	{ kVK_ANSI_Keypad9, "Num 9" },
};

} // namespace

[[nodiscard]] QString KeyName(MacKeyDescriptor descriptor) {
	const auto i = KeyToString.find(descriptor.keycode);
	return (i != end(KeyToString))
		? i->second.utf16()
		: QString("\\x%1").arg(descriptor.keycode, 0, 16);
}

} // namespace base::Platform
