// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/mac/base_info_mac.h"

#include "base/timer.h"
#include "base/algorithm.h"
#include "base/platform/base_platform_info.h"
#include "base/platform/mac/base_utilities_mac.h"

#include <QtCore/QDate>
#include <QtCore/QProcess>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonValue>
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
	for (const QChar &ch : model) {
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
	for (const QString &word : words) {
		if (!result.isEmpty()
			&& word != "Mac"
			&& word != "Book") {
			result.append(' ');
		}
		result.append(word);
	}
	return result;
}

[[nodiscard]] int MajorVersion() {
	static const auto current = QOperatingSystemVersion::current();
	return current.majorVersion();
}

[[nodiscard]] int MinorVersion() {
	static const auto current = QOperatingSystemVersion::current();
	return current.minorVersion();
}

[[nodiscard]] int PatchVersion() {
	static const auto current = QOperatingSystemVersion::current();
	return current.microVersion();
}

template <int Major, int Minor>
bool IsMacThatOrGreater() {
	static const auto result = (MajorVersion() >= Major)
		&& ((MajorVersion() > Major) || (MinorVersion() >= Minor));
	return result;
}

template <int Minor>
[[nodiscard]] bool IsMac10ThatOrGreater() {
	return IsMacThatOrGreater<10, Minor>();
}

[[nodiscard]] NSURL *PrivacySettingsUrl(const QString &section) {
	NSString *url = Q2NSString(
		"x-apple.systempreferences:com.apple.preference.security?" + section
	);
	return [NSURL URLWithString:url];
}

[[nodiscard]] bool RunningThroughRosetta() {
	 auto result = int(0);
	 auto size = sizeof(result);
	 sysctlbyname("sysctl.proc_translated", &result, &size, nullptr, 0);
	 return (result == 1);
}

[[nodiscard]] QString DeviceFromSystemProfiler() {
	// Starting with MacBook M2 the hw.model returns simply Mac[digits],[digits].
	// So we try reading "system_profiler" output.
	auto process = QProcess();
	process.start(
		"system_profiler",
		{ "-json", "SPHardwareDataType", "-detailLevel", "mini" });
	process.waitForFinished();
	auto error = QJsonParseError{ 0, QJsonParseError::NoError };
	const auto document = QJsonDocument::fromJson(process.readAll(), &error);
	if (error.error != QJsonParseError::NoError || !document.isObject()) {
		return {};
	}
	const auto fields = document.object()["SPHardwareDataType"].toArray()[0].toObject();
	const auto result = fields["machine_name"].toString();
	if (result.isEmpty()) {
		return {};
	}
	const auto chip = fields["chip_type"].toString();
	return chip.startsWith("Apple ") ? (result + chip.mid(5)) : result;
}

} // namespace

QString DeviceModelPretty() {
	using namespace base::Platform;
	static const auto result = FinalizeDeviceModel([&] {
		const auto fromSystemProfiler = DeviceFromSystemProfiler();
		if (!fromSystemProfiler.isEmpty()) {
			return fromSystemProfiler;
		}
		size_t length = 0;
		sysctlbyname("hw.model", nullptr, &length, nullptr, 0);
		if (length > 0) {
			QByteArray bytes(length, Qt::Uninitialized);
			sysctlbyname("hw.model", bytes.data(), &length, nullptr, 0);
			const auto parsed = base::CleanAndSimplify(
				FromIdentifier(QString::fromUtf8(bytes)));
			if (!parsed.isEmpty()) {
				return parsed;
			}
		}
		return QString();
	}());
	return result;
}

QString SystemVersionPretty() {
	const auto major = MajorVersion();
	const auto minor = MinorVersion();
	const auto patch = PatchVersion();
	const auto addAsPatch = (patch > 0) ? u".%1"_q.arg(patch) : QString();
	if (major < 10) {
		return "OS X";
	} else if (major == 10 && minor < 12) {
		return QString("OS X 10.%1").arg(minor) + addAsPatch;
	}
	return QString("macOS %1.%2").arg(major).arg(minor) + addAsPatch;
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
	if (!IsMac10_13OrGreater()) {
		return QDate(2023, 7, 1);
	}
	return QDate();
}

int AutoUpdateVersion() {
	if (!IsMac10_13OrGreater()) {
		return 2;
	}
	return 4;
}

QString AutoUpdateKey() {
	if (QSysInfo::currentCpuArchitecture().startsWith("arm")
		|| RunningThroughRosetta()) {
		return "armac";
	} else {
		return "mac";
	}
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

void OpenDesktopCapturePrivacySettings() {
	if (@available(macOS 11.0, *)) {
		CGRequestScreenCaptureAccess();
	}
	[[NSWorkspace sharedWorkspace] openURL:PrivacySettingsUrl("Privacy_ScreenCapture")];
}

void OpenAccessibilityPrivacySettings() {
	NSDictionary *const options=@{(__bridge NSString *)kAXTrustedCheckOptionPrompt: @TRUE};
	AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);
}

} // namespace Platform
