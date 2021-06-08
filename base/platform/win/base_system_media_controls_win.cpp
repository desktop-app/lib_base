// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/base_platform_system_media_controls.h"

#include "base/integration.h"
#include "base/platform/win/base_info_win.h"
#include "base/platform/win/base_windows_wrl.h"
#include "base/platform/win/wrl/wrl_event_h.h"

#include <systemmediatransportcontrolsinterop.h>
#include <windows.media.control.h>
#include <wrl/client.h>

#include <qpa/qplatformnativeinterface.h>

#include <QtCore/QBuffer>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtWidgets/QWidget>

namespace base::Platform {

using namespace Microsoft::WRL;

namespace WinMedia = ABI::Windows::Media;
namespace WinStreams = ABI::Windows::Storage::Streams;

using SMTControls = WinMedia::ISystemMediaTransportControls;
using ButtonsEventArgs =
	WinMedia::ISystemMediaTransportControlsButtonPressedEventArgs;
using DisplayUpdater = WinMedia::ISystemMediaTransportControlsDisplayUpdater;
using MediaType = ABI::Windows::Media::MediaPlaybackType;

struct SystemMediaControls::Private {
	ComPtr<SMTControls> controls;
	ComPtr<DisplayUpdater> displayUpdater;
	ComPtr<WinMedia::IMusicDisplayProperties> displayProperties;
	ComPtr<WinStreams::IDataWriter> iconDataWriter;
	ComPtr<WinStreams::IRandomAccessStream> thumbStream;
	ComPtr<WinStreams::IRandomAccessStreamReference> thumbStreamReference;
	EventRegistrationToken registrationToken;
	bool initialized = false;
	bool hasValidRegistrationToken = false;

	rpl::event_stream<SystemMediaControls::Command> commandRequests;
};

namespace {

using WinPlaybackStatus = ABI::Windows::Media::MediaPlaybackStatus;

WinPlaybackStatus SmtcPlaybackStatus(
		SystemMediaControls::PlaybackStatus status) {
	switch (status) {
	case SystemMediaControls::PlaybackStatus::Playing:
		return WinPlaybackStatus::MediaPlaybackStatus_Playing;
	case SystemMediaControls::PlaybackStatus::Paused:
		return WinPlaybackStatus::MediaPlaybackStatus_Paused;
	case SystemMediaControls::PlaybackStatus::Stopped:
		return WinPlaybackStatus::MediaPlaybackStatus_Stopped;
	}
	Unexpected("SmtcPlaybackStatus in SystemMediaControls");
}

using SMTCButton = WinMedia::SystemMediaTransportControlsButton;

auto SMTCButtonToCommand(SMTCButton button) {
	using Command = SystemMediaControls::Command;

	switch (button) {
	case SMTCButton::SystemMediaTransportControlsButton_Play:
		return Command::Play;
	case SMTCButton::SystemMediaTransportControlsButton_Pause:
		return Command::Pause;
	case SMTCButton::SystemMediaTransportControlsButton_Next:
		return Command::Next;
	case SMTCButton::SystemMediaTransportControlsButton_Previous:
		return Command::Previous;
	case SMTCButton::SystemMediaTransportControlsButton_Stop:
		return Command::Stop;
	case SMTCButton::SystemMediaTransportControlsButton_Record:
	case SMTCButton::SystemMediaTransportControlsButton_FastForward:
	case SMTCButton::SystemMediaTransportControlsButton_Rewind:
	case SMTCButton::SystemMediaTransportControlsButton_ChannelUp:
	case SMTCButton::SystemMediaTransportControlsButton_ChannelDown:
		return Command::None;
	}
	return Command::None;
}

} // namespace

SystemMediaControls::SystemMediaControls()
: _private(std::make_unique<Private>()) {
}

SystemMediaControls::~SystemMediaControls() {
	if (_private->hasValidRegistrationToken) {
		_private->controls->remove_ButtonPressed(_private->registrationToken);
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

	ComPtr<ISystemMediaTransportControlsInterop> interop;
	auto hr = GetActivationFactory(
		StringReferenceWrapper(
			RuntimeClass_Windows_Media_SystemMediaTransportControls).Get(),
		&interop);

	if (FAILED(hr)) {
		return false;
	}

	hr = interop->GetForWindow(
		hwnd,
		IID_PPV_ARGS(&_private->controls));
	if (FAILED(hr)) {
		return false;
	}

	// Buttons handler.
	{
		using Args =
			WinMedia::SystemMediaTransportControlsButtonPressedEventArgs;
		using SMTC = WinMedia::SystemMediaTransportControls;
		using namespace ABI::Windows::Foundation;
		using Handler = ITypedEventHandler<SMTC*, Args*>;

		const auto handler = Callback<Handler>([=](
				not_null<SMTControls*> sender,
				not_null<ButtonsEventArgs*> args) {
			SMTCButton button;
			if (FAILED(args->get_Button(&button))) {
				return E_FAIL;
			}
			Integration::Instance().enterFromEventLoop([&] {
				_private->commandRequests.fire(SMTCButtonToCommand(button));
			});
			return S_OK;
		});

		hr = _private->controls->add_ButtonPressed(
			handler.Get(),
			&_private->registrationToken);

		if (FAILED(hr)) {
			return false;
		}

		_private->hasValidRegistrationToken = true;
	}

	hr = _private->controls->put_IsEnabled(true);
	if (FAILED(hr)) {
		return false;
	}

	hr = _private->controls->get_DisplayUpdater(
		&_private->displayUpdater);
	if (FAILED(hr)) {
		return false;
	}

	hr = _private->displayUpdater->put_Type(
		MediaType::MediaPlaybackType_Music);
	if (FAILED(hr)) {
		return false;
	}

	hr = _private->displayUpdater->get_MusicProperties(
		&_private->displayProperties);
	if (FAILED(hr)) {
		return false;
	}

	_private->initialized = true;
	return true;
}

void SystemMediaControls::setApplicationName(const QString &name) {
}

void SystemMediaControls::setEnabled(bool enabled) {
	_private->controls->put_IsEnabled(enabled);
}

void SystemMediaControls::setIsNextEnabled(bool value) {
	_private->controls->put_IsNextEnabled(value);
}

void SystemMediaControls::setIsPreviousEnabled(bool value) {
	_private->controls->put_IsPreviousEnabled(value);
}

void SystemMediaControls::setIsPlayPauseEnabled(bool value) {
	_private->controls->put_IsPlayEnabled(value);
	_private->controls->put_IsPauseEnabled(value);
}

void SystemMediaControls::setIsStopEnabled(bool value) {
	_private->controls->put_IsStopEnabled(value);
}

void SystemMediaControls::setPlaybackStatus(
		SystemMediaControls::PlaybackStatus status) {
	_private->controls->put_PlaybackStatus(SmtcPlaybackStatus(status));
}

void SystemMediaControls::setTitle(const QString &title) {
	const auto wtitle = title.toStdWString();
	_private->displayProperties->put_Title(
		StringReferenceWrapper(wtitle.data(), wtitle.size()).Get());
}

void SystemMediaControls::setArtist(const QString &artist) {
	const auto wartist = artist.toStdWString();
	_private->displayProperties->put_Artist(
		StringReferenceWrapper(wartist.data(), wartist.size()).Get());
}

void SystemMediaControls::setThumbnail(const QImage &thumbnail) {
	const auto id = StringReferenceWrapper(
		RuntimeClass_Windows_Storage_Streams_InMemoryRandomAccessStream);
	auto hr = base::Platform::ActivateInstance(
		id.Get(),
		&_private->thumbStream);
	if (FAILED(hr)) {
		return;
	}

	ComPtr<WinStreams::IDataWriterFactory> dataWriterFactory;
	hr = GetActivationFactory(
		StringReferenceWrapper(
			RuntimeClass_Windows_Storage_Streams_DataWriter).Get(),
		&dataWriterFactory);
	if (FAILED(hr)) {
		return;
	}

	ComPtr<WinStreams::IOutputStream> outputStream;
	hr = _private->thumbStream.As(&outputStream);
	if (FAILED(hr)) {
		return;
	}

	hr = dataWriterFactory->CreateDataWriter(
		outputStream.Get(),
		&_private->iconDataWriter);
	if (FAILED(hr)) {
		return;
	}

	const auto bitmapRawData = [&] {
		QByteArray bytes;
		QBuffer buffer(&bytes);
		buffer.open(QIODevice::WriteOnly);
		thumbnail.save(&buffer, "JPG", 87);
		buffer.close();
		return std::vector<unsigned char>(bytes.begin(), bytes.end());
	}();
	hr = _private->iconDataWriter->WriteBytes(
		bitmapRawData.size(),
		(BYTE*)bitmapRawData.data());
	if (FAILED(hr)) {
		return;
	}

	using namespace ABI::Windows::Foundation;
	using Handler = IAsyncOperationCompletedHandler<uint32>;
	// Store the written bytes in the stream, an async operation.
	ComPtr<IAsyncOperation<uint32>> storeAsyncOperation;
	hr = _private->iconDataWriter->StoreAsync(&storeAsyncOperation);
	if (FAILED(hr)) {
		return;
	}

	const auto storeAsyncCallback = Callback<Handler>([=](
			not_null<IAsyncOperation<uint32>*> asyncOperation,
			AsyncStatus status) mutable {
		// Check the async operation completed successfully.
		IAsyncInfo *asyncInfo;
		auto hr = asyncOperation->QueryInterface(
			IID_IAsyncInfo,
			reinterpret_cast<void**>(&asyncInfo));
		if (FAILED(hr)) {
			return hr;
		}
		asyncInfo->get_ErrorCode(&hr);
		if (!SUCCEEDED(hr) || !(status == AsyncStatus::Completed)) {
			return hr;
		}
		ComPtr<WinStreams::IRandomAccessStreamReferenceStatics>
			referenceStatics;
		const auto &streamReferenceString =
			RuntimeClass_Windows_Storage_Streams_RandomAccessStreamReference;
		hr = GetActivationFactory(
			StringReferenceWrapper(streamReferenceString).Get(),
			&referenceStatics);
		if (FAILED(hr)) {
			return hr;
		}

		hr = referenceStatics->CreateFromStream(
			_private->thumbStream.Get(),
			&_private->thumbStreamReference);
		if (FAILED(hr)) {
			return hr;
		}

		hr = _private->displayUpdater->put_Thumbnail(
			_private->thumbStreamReference.Get());
		if (FAILED(hr)) {
			return hr;
		}

		hr = _private->displayUpdater->Update();
		if (FAILED(hr)) {
			return hr;
		}
		return hr;
	});

	hr = storeAsyncOperation->put_Completed(storeAsyncCallback.Get());
}

void SystemMediaControls::setDuration(int duration) {
}

void SystemMediaControls::setPosition(int position) {
}

void SystemMediaControls::setVolume(float64 volume) {
}

void SystemMediaControls::clearThumbnail() {
	_private->displayUpdater->put_Thumbnail(nullptr);
	_private->displayUpdater->Update();
}

void SystemMediaControls::clearMetadata() {
	_private->displayUpdater->ClearAll();
	_private->controls->put_IsEnabled(false);
}

void SystemMediaControls::updateDisplay() {
	_private->controls->put_IsEnabled(true);
	_private->displayUpdater->put_Type(MediaType::MediaPlaybackType_Music);
	_private->displayUpdater->Update();
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
