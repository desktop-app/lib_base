// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/mac/base_info_mac.h"

#include "base/timer.h"
#include "base/platform/base_platform_info.h"
#include "base/platform/mac/base_utilities_mac.h"

#include <QtCore/QDate>
#include <QtCore/QJsonObject>
#include <QtCore/QOperatingSystemVersion>
#include <sys/sysctl.h>
#include <Cocoa/Cocoa.h>
#import <IOKit/hidsystem/IOHIDLib.h>

@interface WakeUpObserver : NSObject {
}

- (void) receiveWakeNote:(NSNotification*)note;

@end // @interface WakeUpObserver

@implementation WakeUpObserver {
}

- (void) receiveWakeNote:(NSNotification*)aNotification {
	base::CheckLocalTime();
}

@end // @implementation WakeUpObserver

namespace Platform {
namespace {

WakeUpObserver *GlobalWakeUpObserver = nil;

QString FromIdentifier(const QString &model) {
	if (model.isEmpty() || model.toLower().indexOf("mac") < 0) {
		return QString();
	}
	QStringList words;
	QString word;
	for (const QChar ch : model) {
		if (!ch.isLetter()) {
			continue;
		}
		if (ch.isUpper()) {
			if (!word.isEmpty()) {
				words.push_back(word);
				word = QString();
			}
		}
		word.append(ch);
	}
	if (!word.isEmpty()) {
		words.push_back(word);
	}
	QString result;
	for (const QString word : words) {
		if (!result.isEmpty()
			&& word != "Mac"
			&& word != "Book") {
			result.append(' ');
		}
		result.append(word);
	}
	return result;
}

int MajorVersion() {
	static const auto current = QOperatingSystemVersion::current();
	return current.majorVersion();
}

int MinorVersion() {
	static const auto current = QOperatingSystemVersion::current();
	return current.minorVersion();
}

template <int Major, int Minor>
bool IsMacThatOrGreater() {
	static const auto result = (MajorVersion() >= Major)
		&& ((MajorVersion() > Major) || (MinorVersion() >= Minor));
	return result;
}

template <int Minor>
bool IsMac10ThatOrGreater() {
	return IsMacThatOrGreater<10, Minor>();
}

NSURL *PrivacySettingsUrl(const QString &section) {
	NSString *url = Q2NSString("x-apple.systempreferences:com.apple.preference.security?" + section);
	return [NSURL URLWithString:url];
}

} // namespace

QString DeviceModelPretty() {
	size_t length = 0;
	sysctlbyname("hw.model", nullptr, &length, nullptr, 0);
	if (length > 0) {
		QByteArray bytes(length, Qt::Uninitialized);
		sysctlbyname("hw.model", bytes.data(), &length, nullptr, 0);
		const QString parsed = FromIdentifier(QString::fromUtf8(bytes));
		if (!parsed.isEmpty()) {
			return parsed;
		}
	}
	return "Mac";
}

QString SystemVersionPretty() {
	const auto major = MajorVersion();
	const auto minor = MinorVersion();
	if (major < 10) {
		return "OS X";
	} else if (major == 10 && minor < 12) {
		return QString("OS X 10.%1").arg(minor);
	}
	return QString("macOS %1.%2").arg(major).arg(minor);
}

QString SystemCountry() {
	NSLocale *currentLocale = [NSLocale currentLocale];  // get the current locale.
	NSString *countryCode = [currentLocale objectForKey:NSLocaleCountryCode];
	return countryCode ? NS2QString(countryCode) : QString();
}

QString SystemLanguage() {
	if (auto currentLocale = [NSLocale currentLocale]) { // get the current locale.
		if (NSString *collator = [currentLocale objectForKey:NSLocaleCollatorIdentifier]) {
			return NS2QString(collator);
		}
		if (NSString *identifier = [currentLocale objectForKey:NSLocaleIdentifier]) {
			return NS2QString(identifier);
		}
		if (NSString *language = [currentLocale objectForKey:NSLocaleLanguageCode]) {
			return NS2QString(language);
		}
	}
	return QString();
}

QDate WhenSystemBecomesOutdated() {
	if (!IsMac10_10OrGreater()) {
		return QDate(2019, 9, 1);
	} else if (!IsMac10_12OrGreater()) {
		return QDate(2020, 9, 1);
	}
	return QDate();
}

int AutoUpdateVersion() {
	if (!IsMac10_10OrGreater()) {
		return 1;
	}
	return 2;
}

QString AutoUpdateKey() {
	if (!IsMac10_12OrGreater()) {
		return "osx";
	} else {
		return "mac";
	}
}

bool IsMac10_6OrGreater() {
	return IsMac10ThatOrGreater<6>();
}

bool IsMac10_7OrGreater() {
	return IsMac10ThatOrGreater<7>();
}

bool IsMac10_8OrGreater() {
	return IsMac10ThatOrGreater<8>();
}

bool IsMac10_9OrGreater() {
	return IsMac10ThatOrGreater<9>();
}

bool IsMac10_10OrGreater() {
	return IsMac10ThatOrGreater<10>();
}

bool IsMac10_11OrGreater() {
	return IsMac10ThatOrGreater<11>();
}

bool IsMac10_12OrGreater() {
	return IsMac10ThatOrGreater<12>();
}

bool IsMac10_13OrGreater() {
	return IsMac10ThatOrGreater<13>();
}

bool IsMac10_14OrGreater() {
	return IsMac10ThatOrGreater<14>();
}

bool IsMac10_15OrGreater() {
	return IsMac10ThatOrGreater<15>();
}

bool IsMac11_0OrGreater() {
	return IsMacThatOrGreater<11, 0>();
}

void Start(QJsonObject settings) {
	Expects(GlobalWakeUpObserver == nil);

	GlobalWakeUpObserver = [[WakeUpObserver alloc] init];

	NSNotificationCenter *center = [[NSWorkspace sharedWorkspace] notificationCenter];
	Assert(center != nil);

	[center
		addObserver: GlobalWakeUpObserver
		selector: @selector(receiveWakeNote:)
		name: NSWorkspaceDidWakeNotification
		object: nil];

	Ensures(GlobalWakeUpObserver != nil);
}

void Finish() {
	Expects(GlobalWakeUpObserver != nil);

	[GlobalWakeUpObserver release];
	GlobalWakeUpObserver = nil;
}

void OpenInputMonitoringPrivacySettings() {
	if (@available(macOS 10.15, *)) {
		IOHIDRequestAccess(kIOHIDRequestTypeListenEvent);
	}
	[[NSWorkspace sharedWorkspace] openURL:PrivacySettingsUrl("Privacy_ListenEvent")];
}

void OpenAccessibilityPrivacySettings() {
	NSDictionary *const options=@{(__bridge NSString *)kAXTrustedCheckOptionPrompt: @TRUE};
	AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);
}

} // namespace Platform
