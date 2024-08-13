// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/win/base_info_win.h"

#include "base/algorithm.h"
#include "base/platform/win/base_windows_safe_library.h"

#include <QtCore/QOperatingSystemVersion>
#include <QtCore/QJsonObject>
#include <QtCore/QDate>
#include <QtCore/QSettings>

#include <windows.h>
#include <VersionHelpers.h>

namespace Platform {
namespace {

decltype(&::IsWow64Process2) IsWow64Process2;

bool IsWow64Process2Supported() {
	static const auto Result = [&] {
#define LOAD_SYMBOL(lib, name) base::Platform::LoadMethod(lib, #name, name)
		const auto lib = base::Platform::SafeLoadLibrary(L"Kernel32.dll");
		return LOAD_SYMBOL(lib, IsWow64Process2);
#undef LOAD_SYMBOL
	}();
	return Result;
}

QString GetLangCodeById(unsigned int lngId) {
	const auto primary = (lngId & 0xFFU);
	switch (primary) {
	case 0x36: return u"af"_q;
	case 0x1C: return u"sq"_q;
	case 0x5E: return u"am"_q;
	case 0x01: return u"ar"_q;
	case 0x2B: return u"hy"_q;
	case 0x4D: return u"as"_q;
	case 0x2C: return u"az"_q;
	case 0x45: return u"bn"_q;
	case 0x6D: return u"ba"_q;
	case 0x2D: return u"eu"_q;
	case 0x23: return u"be"_q;
	case 0x1A:
		return (lngId == LANG_CROATIAN)
			? u"hr"_q
			: (lngId == LANG_BOSNIAN_NEUTRAL || lngId == LANG_BOSNIAN)
			? u"bs"_q
			: u"sr"_q;
	case 0x7E: return u"br"_q;
	case 0x02: return u"bg"_q;
	case 0x92: return u"ku"_q;
	case 0x03: return u"ca"_q;
	case 0x04: return u"zh"_q;
	case 0x83: return u"co"_q;
	case 0x05: return u"cs"_q;
	case 0x06: return u"da"_q;
	case 0x65: return u"dv"_q;
	case 0x13: return u"nl"_q;
	case 0x09: return u"en"_q;
	case 0x25: return u"et"_q;
	case 0x38: return u"fo"_q;
	case 0x0B: return u"fi"_q;
	case 0x0c: return u"fr"_q;
	case 0x62: return u"fy"_q;
	case 0x56: return u"gl"_q;
	case 0x37: return u"ka"_q;
	case 0x07: return u"de"_q;
	case 0x08: return u"el"_q;
	case 0x6F: return u"kl"_q;
	case 0x47: return u"gu"_q;
	case 0x68: return u"ha"_q;
	case 0x0D: return u"he"_q;
	case 0x39: return u"hi"_q;
	case 0x0E: return u"hu"_q;
	case 0x0F: return u"is"_q;
	case 0x70: return u"ig"_q;
	case 0x21: return u"id"_q;
	case 0x5D: return u"iu"_q;
	case 0x3C: return u"ga"_q;
	case 0x34: return u"xh"_q;
	case 0x35: return u"zu"_q;
	case 0x10: return u"it"_q;
	case 0x11: return u"ja"_q;
	case 0x4B: return u"kn"_q;
	case 0x3F: return u"kk"_q;
	case 0x53: return u"kh"_q;
	case 0x87: return u"rw"_q;
	case 0x12: return u"ko"_q;
	case 0x40: return u"ky"_q;
	case 0x54: return u"lo"_q;
	case 0x26: return u"lv"_q;
	case 0x27: return u"lt"_q;
	case 0x6E: return u"lb"_q;
	case 0x2F: return u"mk"_q;
	case 0x3E: return u"ms"_q;
	case 0x4C: return u"ml"_q;
	case 0x3A: return u"mt"_q;
	case 0x81: return u"mi"_q;
	case 0x4E: return u"mr"_q;
	case 0x50: return u"mn"_q;
	case 0x61: return u"ne"_q;
	case 0x14: return u"no"_q;
	case 0x82: return u"oc"_q;
	case 0x48: return u"or"_q;
	case 0x63: return u"ps"_q;
	case 0x29: return u"fa"_q;
	case 0x15: return u"pl"_q;
	case 0x16: return u"pt"_q;
	case 0x67: return u"ff"_q;
	case 0x46: return u"pa"_q;
	case 0x18: return u"ro"_q;
	case 0x17: return u"rm"_q;
	case 0x19: return u"ru"_q;
	case 0x3B: return u"se"_q;
	case 0x4F: return u"sa"_q;
	case 0x32: return u"tn"_q;
	case 0x59: return u"sd"_q;
	case 0x5B: return u"si"_q;
	case 0x1B: return u"sk"_q;
	case 0x24: return u"sl"_q;
	case 0x0A: return u"es"_q;
	case 0x41: return u"sw"_q;
	case 0x1D: return u"sv"_q;
	case 0x28: return u"tg"_q;
	case 0x49: return u"ta"_q;
	case 0x44: return u"tt"_q;
	case 0x4A: return u"te"_q;
	case 0x1E: return u"th"_q;
	case 0x51: return u"bo"_q;
	case 0x73: return u"ti"_q;
	case 0x1F: return u"tr"_q;
	case 0x42: return u"tk"_q;
	case 0x22: return u"uk"_q;
	case 0x20: return u"ur"_q;
	case 0x80: return u"ug"_q;
	case 0x43: return u"uz"_q;
	case 0x2A: return u"vi"_q;
	case 0x52: return u"cy"_q;
	case 0x88: return u"wo"_q;
	case 0x78: return u"ii"_q;
	case 0x6A: return u"yo"_q;
	}
	return QString();
}

} // namespace

QString DeviceModelPretty() {
	using namespace base::Platform;
	static const auto result = FinalizeDeviceModel([&] {
		const auto bios = QSettings(
			"HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\BIOS",
			QSettings::NativeFormat);
		const auto value = [&](const char *key) {
			return SimplifyDeviceModel(bios.value(key).toString());
		};

		const auto systemProductName = value("SystemProductName");
		if (const auto model = ProductNameToDeviceModel(systemProductName)
			; !model.isEmpty()) {
			return model;
		}

		const auto systemFamily = value("SystemFamily");
		const auto baseBoardProduct = value("BaseBoardProduct");
		const auto familyBoard = SimplifyDeviceModel(
			systemFamily + ' ' + baseBoardProduct);

		if (IsDeviceModelOk(familyBoard)) {
			return familyBoard;
		} else if (IsDeviceModelOk(baseBoardProduct)) {
			return baseBoardProduct;
		} else if (IsDeviceModelOk(systemFamily)) {
			return systemFamily;
		}
		return QString();
	}());

	return result;
}

QString SystemVersionPretty() {
	static const auto result = [&] {
		QStringList resultList;

		if (IsWindows11OrGreater()) {
			resultList << "Windows 11";
		} else if (IsWindows10OrGreater()) {
			resultList << "Windows 10";
		} else if (IsWindows8Point1OrGreater()) {
			resultList << "Windows 8.1";
		} else if (IsWindows8OrGreater()) {
			resultList << "Windows 8";
		} else if (IsWindows7OrGreater()) {
			resultList << "Windows 7";
		} else {
			resultList << QSysInfo::prettyProductName();
		}

		USHORT processMachine, nativeMachine{};
		if (IsWow64Process2Supported()) {
			IsWow64Process2(
				GetCurrentProcess(),
				&processMachine,
				&nativeMachine);
		} else {
			BOOL isWow64{};
			IsWow64Process(GetCurrentProcess(), &isWow64);
			if (isWow64) {
				nativeMachine = IMAGE_FILE_MACHINE_AMD64;
			}
		}

		switch (nativeMachine) {
		case IMAGE_FILE_MACHINE_AMD64: {
			resultList << "x64";
			break;
		}
		case IMAGE_FILE_MACHINE_ARM64: {
			resultList << "arm64";
			break;
		}
		}

		return resultList.join(' ');
	}();

	return result;
}

QString SystemCountry() {
	auto key = HKEY();
	const auto result = RegOpenKeyEx(
		HKEY_CURRENT_USER,
		L"Control Panel\\International\\Geo",
		0,
		KEY_READ,
		&key);
	if (result == ERROR_SUCCESS) {
		constexpr auto kBufSize = 4;
		auto checkType = DWORD();
		auto checkSize = DWORD(kBufSize * 2);
		auto checkStr = std::array<WCHAR, kBufSize>{ 0 };
		const auto result = RegQueryValueEx(
			key,
			L"Name",
			0,
			&checkType,
			reinterpret_cast<BYTE*>(checkStr.data()),
			&checkSize);
		if (result == ERROR_SUCCESS && checkSize == 6) { // 2 wchars + null
			return QString::fromWCharArray(checkStr.data());
		}
	}

	int chCount = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, 0, 0);
	if (chCount && chCount < 128) {
		WCHAR wstrCountry[128];
		int len = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, wstrCountry, chCount);
		if (len) {
			return QString::fromStdWString(std::wstring(wstrCountry));
		}
	}
	return QString();
}

QString SystemLanguage() {
	constexpr auto kMaxLanguageLength = 128;

	auto uiLanguageId = GetUserDefaultUILanguage();
	auto uiLanguageLength = GetLocaleInfo(uiLanguageId, LOCALE_SNAME, nullptr, 0);
	if (uiLanguageLength > 0 && uiLanguageLength < kMaxLanguageLength) {
		WCHAR uiLanguageWideString[kMaxLanguageLength] = { 0 };
		uiLanguageLength = GetLocaleInfo(uiLanguageId, LOCALE_SNAME, uiLanguageWideString, uiLanguageLength);
		if (uiLanguageLength <= 0) {
			return QString();
		}
		return QString::fromWCharArray(uiLanguageWideString);
	}
	auto uiLanguageCodeLength = GetLocaleInfo(uiLanguageId, LOCALE_ILANGUAGE, nullptr, 0);
	if (uiLanguageCodeLength > 0 && uiLanguageCodeLength < kMaxLanguageLength) {
		WCHAR uiLanguageCodeWideString[kMaxLanguageLength] = { 0 };
		uiLanguageCodeLength = GetLocaleInfo(uiLanguageId, LOCALE_ILANGUAGE, uiLanguageCodeWideString, uiLanguageCodeLength);
		if (uiLanguageCodeLength <= 0) {
			return QString();
		}

		auto languageCode = 0U;
		for (auto i = 0; i != uiLanguageCodeLength; ++i) {
			auto ch = uiLanguageCodeWideString[i];
			if (!ch) {
				break;
			}
			languageCode *= 0x10U;
			if (ch >= WCHAR('0') && ch <= WCHAR('9')) {
				languageCode += static_cast<unsigned>(int(ch) - int(WCHAR('0')));
			} else if (ch >= WCHAR('A') && ch <= WCHAR('F')) {
				languageCode += static_cast<unsigned>(0x0A + int(ch) - int(WCHAR('A')));
			} else {
				return QString();
			}
		}
		return GetLangCodeById(languageCode);
	}
	return QString();
}

QDate WhenSystemBecomesOutdated() {
	return QDate();
}

int AutoUpdateVersion() {
	return 4;
}

QString AutoUpdateKey() {
	if (IsWindowsARM64()) {
		return "winarm";
	} else if (IsWindows64Bit()) {
		return "win64";
	} else {
		return "win";
	}
}

bool IsWindows7OrGreater() {
	static const auto result = ::IsWindows7OrGreater();
	return result;
}

bool IsWindows8OrGreater() {
	static const auto result = ::IsWindows8OrGreater();
	return result;
}

bool IsWindows8Point1OrGreater() {
	static const auto result = ::IsWindows8Point1OrGreater();
	return result;
}

bool IsWindows10OrGreater() {
	static const auto result = ::IsWindows10OrGreater();
	return result;
}

bool IsWindows11OrGreater() {
	static const auto result = [&] {
		if (!IsWindows10OrGreater()) {
			return false;
		}
		const auto version = QOperatingSystemVersion::current();
		return (version.majorVersion() > 10)
			|| (version.microVersion() >= 22000);
	}();
	return result;
}

void Start(QJsonObject settings) {
	SetDllDirectory(L"");
}

void Finish() {
}

} // namespace Platform
