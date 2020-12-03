// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:z
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/platform/base_platform_global_shortcuts.h"
#include "base/platform/win/base_windows_key_codes.h"
#include "base/flat_map.h"
#include "base/flat_set.h"

#include <QtCore/QObject>

namespace base::Platform {

class GlobalShortcutValueWin final : public GlobalShortcutValue {
public:
	GlobalShortcutValueWin(std::vector<WinKeyDescriptor> descriptors);

	QString toDisplayString() override;
	QByteArray serialize() override;

	const std::vector<WinKeyDescriptor> &descriptors() const {
		return _descriptors;
	}

private:
	std::vector<WinKeyDescriptor> _descriptors;

};

class GlobalShortcutManagerWin final
	: public GlobalShortcutManager
	, public QObject {
public:
	GlobalShortcutManagerWin();
	~GlobalShortcutManagerWin();

	void startRecording(
		Fn<void(GlobalShortcut)> progress,
		Fn<void(GlobalShortcut)> done) override;
	void stopRecording() override;
	void startWatching(
		GlobalShortcut shortcut,
		Fn<void(bool pressed)> callback) override;
	void stopWatching(GlobalShortcut shortcut) override;

	GlobalShortcut shortcutFromSerialized(QByteArray serialized) override;

	void schedule(WinKeyDescriptor descriptor, bool down);

private:
	struct Watch {
		GlobalShortcut shortcut;
		std::vector<WinKeyDescriptor> sorted;
		Fn<void(bool pressed)> callback;
	};
	void process(WinKeyDescriptor descriptor, bool down);
	void processRecording(WinKeyDescriptor descriptor, bool down);
	void processRecordingPress(WinKeyDescriptor descriptor);
	void processRecordingRelease(WinKeyDescriptor descriptor);
	void finishRecording();

	Fn<void(GlobalShortcut)> _recordingProgress;
	Fn<void(GlobalShortcut)> _recordingDone;
	std::vector<WinKeyDescriptor> _recordingDown;
	base::flat_set<WinKeyDescriptor> _recordingUp;
	base::flat_set<WinKeyDescriptor> _down;
	std::vector<Watch> _watchlist;
	std::vector<GlobalShortcut> _pressed;

	bool _recording = false;

};

} // namespace base::Platform
