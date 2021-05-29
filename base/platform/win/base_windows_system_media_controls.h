// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/platform/win/base_windows_h.h"

namespace base::Platform {

class SystemMediaControlsWin {
public:
	enum class Command {
		Play,
		Pause,
		Next,
		Previous,
		Stop,
		None,
	};

	enum class PlaybackStatus {
		Playing,
		Paused,
		Stopped,
	};

	SystemMediaControlsWin();
	~SystemMediaControlsWin();

	bool init(HWND hwnd);

	void setEnabled(bool enabled);
	void setIsNextEnabled(bool value);
	void setIsPreviousEnabled(bool value);
	void setIsPlayPauseEnabled(bool value);
	void setIsStopEnabled(bool value);
	void setPlaybackStatus(PlaybackStatus status);
	void setTitle(const QString &title);
	void setArtist(const QString &artist);
	void setThumbnail(const QImage &thumbnail);
	void clearThumbnail();
	void clearMetadata();
	void updateDisplay();

	[[nodiscard]] rpl::producer<Command> commandRequests() const;

private:
	struct Private;

	const std::unique_ptr<Private> _private;

	bool _hasValidRegistrationToken = false;
	bool _initialized = false;

	rpl::event_stream<Command> _commandRequests;
};

}  // namespace base::Platform
