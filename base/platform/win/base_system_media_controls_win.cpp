// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/base_platform_system_media_controls.h"

#include "unknwn.h" // Conversion from winrt::guid_of to GUID.

#include "base/integration.h"
#include "base/platform/win/base_info_win.h"
#include "base/platform/win/base_windows_winrt.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Storage.Streams.h>

#include <systemmediatransportcontrolsinterop.h>

#include <qpa/qplatformnativeinterface.h>

#include <QtCore/QBuffer>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtWidgets/QWidget>

namespace winrt {
namespace Streams {
	using namespace Windows::Storage::Streams;
} // namespace Streams
namespace Media {
	using namespace Windows::Media;
} // namespace Media
} // namespace winrt

namespace base::Platform {

struct SystemMediaControls::Private {
	using IReferenceStatics
		= winrt::Streams::IRandomAccessStreamReferenceStatics;
	Private()
	: controls(nullptr)
	, referenceStatics(winrt::get_activation_factory
		<winrt::Streams::RandomAccessStreamReference,
		IReferenceStatics>()) {
	}
	winrt::Media::SystemMediaTransportControls controls;
	winrt::Media::ISystemMediaTransportControlsDisplayUpdater displayUpdater;
	winrt::Media::IMusicDisplayProperties displayProperties;
	winrt::Streams::DataWriter iconDataWriter;
	const IReferenceStatics referenceStatics;
	winrt::event_token eventToken;
	bool initialized = false;

	rpl::event_stream<SystemMediaControls::Command> commandRequests;
};

namespace {

winrt::Media::MediaPlaybackStatus SmtcPlaybackStatus(
		SystemMediaControls::PlaybackStatus status) {
	switch (status) {
	case SystemMediaControls::PlaybackStatus::Playing:
		return winrt::Media::MediaPlaybackStatus::Playing;
	case SystemMediaControls::PlaybackStatus::Paused:
		return winrt::Media::MediaPlaybackStatus::Paused;
	case SystemMediaControls::PlaybackStatus::Stopped:
		return winrt::Media::MediaPlaybackStatus::Stopped;
	}
	Unexpected("SmtcPlaybackStatus in SystemMediaControls");
}


auto SMTCButtonToCommand(
		winrt::Media::SystemMediaTransportControlsButton button) {
	using SMTCButton = winrt::Media::SystemMediaTransportControlsButton;
	using Command = SystemMediaControls::Command;

	switch (button) {
	case SMTCButton::Play:
		return Command::Play;
	case SMTCButton::Pause:
		return Command::Pause;
	case SMTCButton::Next:
		return Command::Next;
	case SMTCButton::Previous:
		return Command::Previous;
	case SMTCButton::Stop:
		return Command::Stop;
	case SMTCButton::Record:
	case SMTCButton::FastForward:
	case SMTCButton::Rewind:
	case SMTCButton::ChannelUp:
	case SMTCButton::ChannelDown:
		return Command::None;
	}
	return Command::None;
}

} // namespace

SystemMediaControls::SystemMediaControls()
: _private(std::make_unique<Private>()) {
}

SystemMediaControls::~SystemMediaControls() {
	if (_private->eventToken) {
		_private->controls.ButtonPressed(base::take(_private->eventToken));
		clearMetadata();
	}
}

bool SystemMediaControls::init(std::optional<QWidget*> parent) {
	if (_private->initialized) {
		return _private->initialized;
	}
	if (!parent.has_value()) {
		return false;
	}
	const auto window = (*parent)->window()->windowHandle();
	const auto native = QGuiApplication::platformNativeInterface();
	if (!window || !native) {
		return false;
	}

	// Should be moved to separated file.
	const auto hwnd = static_cast<HWND>(native->nativeResourceForWindow(
		QByteArrayLiteral("handle"),
		window));

	const auto interop = winrt::get_activation_factory
		<winrt::Media::SystemMediaTransportControls,
		ISystemMediaTransportControlsInterop>();

	winrt::com_ptr<winrt::Media::ISystemMediaTransportControls> icontrols;
	auto hr = interop->GetForWindow(
		hwnd,
		winrt::guid_of<winrt::Media::ISystemMediaTransportControls>(),
		icontrols.put_void());

	if (FAILED(hr)) {
		return false;
	}

	_private->controls = winrt::Media::SystemMediaTransportControls(
		icontrols.detach(),
		winrt::take_ownership_from_abi);

	// Buttons handler.
	using ButtonsEventArgs =
		winrt::Media::SystemMediaTransportControlsButtonPressedEventArgs;
	_private->eventToken = _private->controls.ButtonPressed([=](
		const auto &sender,
		const ButtonsEventArgs &args) {
			// This lambda is called in a non-main thread.
			const auto button = args.Button();
			crl::on_main([=] {
				_private->commandRequests.fire(SMTCButtonToCommand(button));
			});
		});

	_private->controls.IsEnabled(true);

	_private->displayUpdater = _private->controls.DisplayUpdater();
	_private->displayUpdater.Type(winrt::Media::MediaPlaybackType::Music);
	_private->displayProperties = _private->displayUpdater.MusicProperties();

	_private->initialized = true;
	return true;
}

void SystemMediaControls::setApplicationName(const QString &name) {
}

void SystemMediaControls::setEnabled(bool enabled) {
	_private->controls.IsEnabled(enabled);
}

void SystemMediaControls::setIsNextEnabled(bool value) {
	_private->controls.IsNextEnabled(value);
}

void SystemMediaControls::setIsPreviousEnabled(bool value) {
	_private->controls.IsPreviousEnabled(value);
}

void SystemMediaControls::setIsPlayPauseEnabled(bool value) {
	_private->controls.IsPlayEnabled(value);
	_private->controls.IsPauseEnabled(value);
}

void SystemMediaControls::setIsStopEnabled(bool value) {
	_private->controls.IsStopEnabled(value);
}

void SystemMediaControls::setPlaybackStatus(
		SystemMediaControls::PlaybackStatus status) {
	_private->controls.PlaybackStatus(SmtcPlaybackStatus(status));
}

void SystemMediaControls::setTitle(const QString &title) {
	const auto htitle = winrt::to_hstring(title.toStdString());
	_private->displayProperties.Title(htitle);
}

void SystemMediaControls::setArtist(const QString &artist) {
	const auto hartist = winrt::to_hstring(artist.toStdString());
	_private->displayProperties.Artist(hartist);
}

void SystemMediaControls::setThumbnail(const QImage &thumbnail) {
	auto thumbStream = winrt::Streams::InMemoryRandomAccessStream();
	_private->iconDataWriter = winrt::Streams::DataWriter(thumbStream);

	const auto bitmapRawData = [&] {
		QByteArray bytes;
		QBuffer buffer(&bytes);
		buffer.open(QIODevice::WriteOnly);
		thumbnail.save(&buffer, "JPG", 87);
		buffer.close();
		return std::vector<unsigned char>(bytes.begin(), bytes.end());
	}();
	_private->iconDataWriter.WriteBytes(bitmapRawData);

	using namespace winrt::Windows;
	_private->iconDataWriter.StoreAsync().Completed([=,
			thumbStream = std::move(thumbStream)](
		Foundation::IAsyncOperation<uint32> asyncOperation,
		Foundation::AsyncStatus status) {

		// Check the async operation completed successfully.
		if ((status != Foundation::AsyncStatus::Completed)
			|| FAILED(asyncOperation.ErrorCode())) {
			return;
		}

		auto reference = _private->referenceStatics.CreateFromStream(
			thumbStream);

		_private->displayUpdater.Thumbnail(reference);
		_private->displayUpdater.Update();
	});
}

void SystemMediaControls::setDuration(int duration) {
}

void SystemMediaControls::setPosition(int position) {
}

void SystemMediaControls::setVolume(float64 volume) {
}

void SystemMediaControls::clearThumbnail() {
	_private->displayUpdater.Thumbnail(nullptr);
	_private->displayUpdater.Update();
}

void SystemMediaControls::clearMetadata() {
	_private->displayUpdater.ClearAll();
	_private->controls.IsEnabled(false);
}

void SystemMediaControls::updateDisplay() {
	_private->controls.IsEnabled(true);
	_private->displayUpdater.Type(winrt::Media::MediaPlaybackType::Music);
	_private->displayUpdater.Update();
}

auto SystemMediaControls::commandRequests() const
-> rpl::producer<SystemMediaControls::Command> {
	return _private->commandRequests.events();
}

rpl::producer<float64> SystemMediaControls::seekRequests() const {
	return rpl::never<float64>();
}

rpl::producer<float64> SystemMediaControls::volumeChangeRequests() const {
	return rpl::never<float64>();
}

rpl::producer<> SystemMediaControls::updatePositionRequests() const {
	return rpl::never<>();
}

bool SystemMediaControls::seekingSupported() const {
	return false;
}

bool SystemMediaControls::volumeSupported() const {
	return false;
}

bool SystemMediaControls::Supported() {
	return ::Platform::IsWindows10OrGreater();
}

} // namespace base::Platform
