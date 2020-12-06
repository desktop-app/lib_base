// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/platform/base_platform_global_shortcuts.h"
#include "base/platform/mac/base_mac_key_codes.h"
#include "base/flat_map.h"
#include "base/flat_set.h"

#include <QtCore/QObject>

namespace base::Platform {

class GlobalShortcutValueMac final : public GlobalShortcutValue {
public:
	GlobalShortcutValueMac(std::vector<MacKeyDescriptor> descriptors);

	QString toDisplayString() override;
	QByteArray serialize() override;

	const std::vector<MacKeyDescriptor> &descriptors() const {
		return _descriptors;
	}

private:
	std::vector<MacKeyDescriptor> _descriptors;

};

class GlobalShortcutManagerMac final
	: public GlobalShortcutManager
	, public QObject {
public:
	GlobalShortcutManagerMac();
	~GlobalShortcutManagerMac();

	void startRecording(
		Fn<void(GlobalShortcut)> progress,
		Fn<void(GlobalShortcut)> done) override;
	void stopRecording() override;
	void startWatching(
		GlobalShortcut shortcut,
		Fn<void(bool pressed)> callback) override;
	void stopWatching(GlobalShortcut shortcut) override;

	GlobalShortcut shortcutFromSerialized(QByteArray serialized) override;

	void schedule(MacKeyDescriptor descriptor, bool down);

private:
	struct Watch {
		GlobalShortcut shortcut;
		std::vector<MacKeyDescriptor> sorted;
		Fn<void(bool pressed)> callback;
	};
	void process(MacKeyDescriptor descriptor, bool down);
	void processRecording(MacKeyDescriptor descriptor, bool down);
	void processRecordingPress(MacKeyDescriptor descriptor);
	void processRecordingRelease(MacKeyDescriptor descriptor);
	void finishRecording();

	Fn<void(GlobalShortcut)> _recordingProgress;
	Fn<void(GlobalShortcut)> _recordingDone;
	std::vector<MacKeyDescriptor> _recordingDown;
	base::flat_set<MacKeyDescriptor> _recordingUp;
	base::flat_set<MacKeyDescriptor> _down;
	std::vector<Watch> _watchlist;
	std::vector<GlobalShortcut> _pressed;

	bool _recording = false;
	bool _valid = false;

};

} // namespace base::Platform
