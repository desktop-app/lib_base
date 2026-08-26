// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/base_platform_system_media_controls.h"

#include <unknwn.h> // Conversion from winrt::guid_of to GUID.

#include "base/integration.h"
#include "base/platform/win/base_info_win.h"
#include "base/platform/win/base_windows_winrt.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Storage.Streams.h>

#include <systemmediatransportcontrolsinterop.h>

#include <QtCore/QBuffer>
#include <QtGui/QImage>

#include <windows.h>

namespace winrt {
namespace Streams {
	using namespace Windows::Storage::Streams;
} // namespace Streams
namespace Media {
	using namespace Windows::Media;
} // namespace Media
} // namespace winrt

namespace base::Platform {
namespace {

constexpr auto kWindowClassName = L"DesktopAppSystemMediaControls";

struct ControlsWindow {
	~ControlsWindow();

	HWND hwnd = nullptr;
	Fn<void()> activated;
};

ControlsWindow::~ControlsWindow() {
	if (hwnd) {
		DestroyWindow(hwnd);
	}
}

LRESULT CALLBACK ControlsWndProc(
		HWND hwnd,
		UINT message,
		WPARAM wParam,
		LPARAM lParam) {
	if (message == WM_NCCREATE) {
		const auto create = reinterpret_cast<CREATESTRUCTW*>(lParam);
		SetWindowLongPtrW(
			hwnd,
			GWLP_USERDATA,
			reinterpret_cast<LONG_PTR>(create->lpCreateParams));
	} else if (message == WM_NCACTIVATE && wParam) {
		const auto that = reinterpret_cast<ControlsWindow*>(
			GetWindowLongPtrW(hwnd, GWLP_USERDATA));
		if (that && that->activated) {
			that->activated();
		}
	}
	return DefWindowProcW(hwnd, message, wParam, lParam);
}

[[nodiscard]] bool RegisterWindowClass() {
	static const auto result = [] {
		auto descriptor = WNDCLASSEXW();
		descriptor.cbSize = sizeof(descriptor);
		descriptor.lpfnWndProc = ControlsWndProc;
		descriptor.hInstance = GetModuleHandleW(nullptr);
		descriptor.lpszClassName = kWindowClassName;
		return RegisterClassExW(&descriptor)
			|| (GetLastError() == ERROR_CLASS_ALREADY_EXISTS);
	}();
	return result;
}

// The controls are attached to this window by GetForWindow and the system
// activates it when the user clicks the media flyout, which arrives here
// as WM_NCACTIVATE - so it has to be a real top-level window and can't be
// a message-only one. It is created without WS_VISIBLE and is never shown,
// so the compositor never gets a visual for it.
//
// A hidden top-level QWidget was used here before. Even though it was
// never shown either, it could still end up composited on screen as an
// unpaintable white rectangle while Windows considered it hidden.
[[nodiscard]] HWND CreateControlsWindow(ControlsWindow *data) {
	if (!RegisterWindowClass()) {
		return nullptr;
	}
	return CreateWindowExW(
		WS_EX_TOOLWINDOW,
		kWindowClassName,
		nullptr,
		WS_POPUP,
		0,
		0,
		0,
		0,
		nullptr,
		nullptr,
		GetModuleHandleW(nullptr),
		data);
}

} // namespace

struct SystemMediaControls::Private {
	using IReferenceStatics
		= winrt::Streams::IRandomAccessStreamReferenceStatics;
	Private()
	: controls(nullptr)
	// Unwrapped, this threw straight out of the constructor: WinRT maps an
	// E_OUTOFMEMORY from the out-of-process server to std::bad_alloc, which
	// nothing here catches. The only use of referenceStatics is already
	// inside a Try(), so a null one degrades instead of aborting.
	, referenceStatics(WinRT::Try([] {
		return winrt::get_activation_factory<
			winrt::Streams::RandomAccessStreamReference,
			IReferenceStatics>();
	}).value_or(IReferenceStatics(nullptr))) {
	}

	ControlsWindow window;
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

bool SystemMediaControls::init() {
	if (_private->initialized) {
		return _private->initialized;
	}
	const auto hwnd = _private->window.hwnd
		? _private->window.hwnd
		: CreateControlsWindow(&_private->window);
	if (!hwnd) {
		return false;
	}
	_private->window.hwnd = hwnd;

	const auto interop = WinRT::Try([&] {
		return winrt::get_activation_factory<
			winrt::Media::SystemMediaTransportControls,
			ISystemMediaTransportControlsInterop>();
	}).value_or(nullptr);
	if (!interop) {
		return false;
	}

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

	using ButtonsEventArgs =
		winrt::Media::SystemMediaTransportControlsButtonPressedEventArgs;
	const auto result = WinRT::Try([&] {
		// Buttons handler.
		_private->eventToken = _private->controls.ButtonPressed([=](
				const auto &sender,
				const ButtonsEventArgs &args) {
			// This lambda is called in a non-main thread.
			crl::on_main([=] {
				const auto button = WinRT::Try([&] {
					return SMTCButtonToCommand(args.Button());
				}).value_or(SystemMediaControls::Command::None);
				_private->commandRequests.fire_copy(button);
			});
		});

		_private->controls.IsEnabled(true);

		auto displayUpdater = _private->controls.DisplayUpdater();
		displayUpdater.Type(winrt::Media::MediaPlaybackType::Music);
		_private->displayProperties = displayUpdater.MusicProperties();
		_private->displayUpdater = std::move(displayUpdater);
	});

	_private->initialized = result;
	if (result) {
		const auto raw = _private.get();
		_private->window.activated = [=] {
			base::Integration::Instance().enterFromEventLoop([&] {
				raw->commandRequests.fire(Command::Raise);
			});
		};
	}
	return result;
}

void SystemMediaControls::setApplicationName(const QString &name) {
}

void SystemMediaControls::setEnabled(bool enabled) {
	WinRT::Try([&] { _private->controls.IsEnabled(enabled); });
}

void SystemMediaControls::setIsNextEnabled(bool value) {
	WinRT::Try([&] { _private->controls.IsNextEnabled(value); });
}

void SystemMediaControls::setIsPreviousEnabled(bool value) {
	WinRT::Try([&] { _private->controls.IsPreviousEnabled(value); });
}

void SystemMediaControls::setIsPlayPauseEnabled(bool value) {
	WinRT::Try([&] {
		_private->controls.IsPlayEnabled(value);
		_private->controls.IsPauseEnabled(value);
	});
}

void SystemMediaControls::setIsStopEnabled(bool value) {
	WinRT::Try([&] { _private->controls.IsStopEnabled(value); });
}

void SystemMediaControls::setPlaybackStatus(
		SystemMediaControls::PlaybackStatus status) {
	WinRT::Try([&] {
		_private->controls.PlaybackStatus(SmtcPlaybackStatus(status));
	});
}

void SystemMediaControls::setLoopStatus(LoopStatus status) {
}

void SystemMediaControls::setShuffle(bool value) {
}

void SystemMediaControls::setTitle(const QString &title) {
	const auto htitle = winrt::to_hstring(title.toStdString());
	WinRT::Try([&] { _private->displayProperties.Title(htitle); });
}

void SystemMediaControls::setArtist(const QString &artist) {
	const auto hartist = winrt::to_hstring(artist.toStdString());
	WinRT::Try([&] { _private->displayProperties.Artist(hartist); });
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

	WinRT::Try([&] {
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

			if (!_private->referenceStatics) {
				// The activation factory failed in the constructor. Calling
				// through a null projected interface reads a vtable pointer
				// out of nullptr, which is an access violation Try() cannot
				// catch - so the artwork is simply skipped.
				return;
			}
			WinRT::Try([&] {
				_private->displayUpdater.Thumbnail(
					_private->referenceStatics.CreateFromStream(thumbStream));
				_private->displayUpdater.Update();
			});
		});
	});
}

void SystemMediaControls::setDuration(int duration) {
}

void SystemMediaControls::setPosition(int position) {
}

void SystemMediaControls::setPlaybackRate(float64 rate) {
}

void SystemMediaControls::setVolume(float64 volume) {
}

void SystemMediaControls::clearThumbnail() {
	WinRT::Try([&] {
		_private->displayUpdater.Thumbnail(nullptr);
		_private->displayUpdater.Update();
	});
}

void SystemMediaControls::clearMetadata() {
	WinRT::Try([&] {
		_private->displayUpdater.ClearAll();
		_private->controls.IsEnabled(false);
	});
}

void SystemMediaControls::updateDisplay() {
	WinRT::Try([&] {
		_private->controls.IsEnabled(true);
		_private->displayUpdater.Type(winrt::Media::MediaPlaybackType::Music);
		_private->displayUpdater.Update();
	});
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
