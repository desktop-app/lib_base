// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/base_platform_system_media_controls.h"

#include "base/integration.h"
#include "base/platform/base_platform_info.h" // IsWayland

#include <mpris/mpris.hpp>

#include <QtCore/QBuffer>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <ksandbox.h>

namespace base::Platform {
namespace {

using namespace gi::repository;
namespace GObject = gi::repository::GObject;

// QString to GLib::Variant.
inline auto Q2V(const QString &s) {
	return GLib::Variant::new_string(s.toStdString());
}

std::string ConvertPlaybackStatus(
		SystemMediaControls::PlaybackStatus status) {
	using Status = SystemMediaControls::PlaybackStatus;
	switch (status) {
	case Status::Playing: return "Playing";
	case Status::Paused: return "Paused";
	case Status::Stopped: return "Stopped";
	}
	Unexpected("ConvertPlaybackStatus in SystemMediaControls");
}

std::string ConvertLoopStatus(SystemMediaControls::LoopStatus status) {
	using Status = SystemMediaControls::LoopStatus;
	switch (status) {
	case Status::None: return "None";
	case Status::Track: return "Track";
	case Status::Playlist: return "Playlist";
	}
	Unexpected("ConvertLoopStatus in SystemMediaControls");
}

auto EventToCommand(const std::string &event) {
	using Command = SystemMediaControls::Command;
	if (event == "Pause") {
		return Command::Pause;
	} else if (event == "Play") {
		return Command::Play;
	} else if (event == "Stop") {
		return Command::Stop;
	} else if (event == "PlayPause") {
		return Command::PlayPause;
	} else if (event == "Next") {
		return Command::Next;
	} else if (event == "Previous") {
		return Command::Previous;
	} else if (event == "Quit") {
		return Command::Quit;
	} else if (event == "Raise") {
		return Command::Raise;
	}
	Unexpected("EventToCommand in SystemMediaControls");
}

auto LoopStatusToCommand(const std::string &status) {
	using Command = SystemMediaControls::Command;
	if (status == "None") {
		return Command::LoopNone;
	} else if (status == "Track") {
		return Command::LoopTrack;
	} else if (status == "Playlist") {
		return Command::LoopPlaylist;
	}
	return Command::None;
}

} // namespace

struct SystemMediaControls::Private : public Mpris::MediaPlayer2 {
public:
	struct PlayerData {
		int64 duration = 0;
	};

	Private();

	void init();
	void deinit();

	[[nodiscard]] bool dbusAvailable() {
		return static_cast<bool>(_dbus.connection);
	}

	[[nodiscard]] Mpris::MediaPlayer2Player player() {
		return *_player;
	}

	[[nodiscard]] PlayerData &playerData() {
		return _playerData;
	}

	[[nodiscard]] rpl::producer<Command> commandRequests() const {
		return _commandRequests.events();
	}

	[[nodiscard]] rpl::producer<int64> seekRequests() const {
		return _seekRequests.events();
	}

	[[nodiscard]] rpl::producer<float64> volumeChangeRequests() const {
		return _volumeChangeRequests.events();
	}

	[[nodiscard]] rpl::producer<> updatePositionRequests() const {
		return _updatePositionRequests.events();
	}

private:
	class Player : public Mpris::impl::MediaPlayer2PlayerSkeletonImpl {
	public:
		Player(not_null<Private*> parent)
		: Mpris::impl::MediaPlayer2PlayerSkeletonImpl(typeid(*this))
		, _parent(parent)
		, _position(this) {
		}

	private:
		const not_null<Private*> _parent;

		class Position : public gi::property<int64> {
		public:
			Position(not_null<Player*> parent)
			: gi::property<int64>(parent, "position")
			, _parent(parent) {
			}

		protected:
			void get_property(GValue *value) override {
				// prevent recursion via SystemMediaControls::setPosition
				if (!_updating) {
					_updating = true;
					_parent->_parent->_updatePositionRequests.fire({});
					_updating = false;
				}
				gi::property<int64>::get_property(value);
			}

		private:
			const not_null<Player*> _parent;
			bool _updating = false;
		} _position;
	};

	gi::ref_ptr<Player> _player;

	struct {
		Gio::DBusConnection connection;
		Gio::DBusObjectManagerServer objectManager;
		uint ownId = 0;
	} _dbus;

	PlayerData _playerData;

	rpl::event_stream<Command> _commandRequests;
	rpl::event_stream<int64> _seekRequests;
	rpl::event_stream<float64> _volumeChangeRequests;
	rpl::event_stream<> _updatePositionRequests;
};

SystemMediaControls::Private::Private()
: Mpris::MediaPlayer2(Mpris::MediaPlayer2Skeleton::new_())
, _player(gi::make_ref<Player>(this))
, _dbus({
	.connection = Gio::bus_get_sync(Gio::BusType::SESSION_, nullptr)
}) {
	set_can_quit(true);
	set_can_raise(!::Platform::IsWayland());
	set_desktop_entry(QGuiApplication::desktopFileName().toStdString());
	set_identity(QGuiApplication::desktopFileName().toStdString());
	player().set_can_control(true);
	player().set_can_seek(true);
	player().set_maximum_rate(1.0);
	player().set_minimum_rate(1.0);
	player().set_playback_status("Stopped");
	player().set_loop_status("None");
	player().set_rate(1.0);
	const auto executeCommand = [=](
			GObject::Object,
			Gio::DBusMethodInvocation invocation) {
		base::Integration::Instance().enterFromEventLoop([&] {
			_commandRequests.fire_copy(
				EventToCommand(invocation.get_method_name()));
		});
		invocation.return_value();
		return true;
	};
	signal_handle_quit().connect(executeCommand);
	signal_handle_raise().connect(executeCommand);
	player().signal_handle_next().connect(executeCommand);
	player().signal_handle_pause().connect(executeCommand);
	player().signal_handle_play().connect(executeCommand);
	player().signal_handle_play_pause().connect(executeCommand);
	player().signal_handle_previous().connect(executeCommand);
	player().signal_handle_stop().connect(executeCommand);
	player().signal_handle_seek().connect([=](
			Mpris::MediaPlayer2Player,
			Gio::DBusMethodInvocation invocation,
			int64 offset) {
		base::Integration::Instance().enterFromEventLoop([&] {
			_seekRequests.fire_copy(
				player().property_position().get() + offset);
		});
		player().complete_seek(invocation);
		return true;
	});
	player().signal_handle_set_position().connect([=](
			Mpris::MediaPlayer2Player,
			Gio::DBusMethodInvocation invocation,
			const std::string &trackId,
			int64 position) {
		base::Integration::Instance().enterFromEventLoop([&] {
			_seekRequests.fire_copy(position);
		});
		player().complete_set_position(invocation);
		return true;
	});
	player().property_loop_status().signal_notify().connect([=](
			GObject::Object,
			GObject::ParamSpec) {
		base::Integration::Instance().enterFromEventLoop([&] {
			_commandRequests.fire_copy(
				LoopStatusToCommand(player().get_loop_status()));
		});
	});
	player().property_shuffle().signal_notify().connect([=](
			GObject::Object,
			GObject::ParamSpec) {
		base::Integration::Instance().enterFromEventLoop([&] {
			_commandRequests.fire_copy(Command::Shuffle);
		});
	});
	player().property_volume().signal_notify().connect([=](
			GObject::Object,
			GObject::ParamSpec) {
		base::Integration::Instance().enterFromEventLoop([&] {
			_volumeChangeRequests.fire_copy(player().get_volume());
		});
	});
}

void SystemMediaControls::Private::init() {
	if (!_dbus.connection || _dbus.ownId) {
		return;
	}
	auto object = Mpris::ObjectSkeleton::new_("/org/mpris/MediaPlayer2");
	object.set_media_player2(*this);
	object.set_media_player2_player(player());
	_dbus.objectManager = Gio::DBusObjectManagerServer::new_("/org/mpris");
	_dbus.objectManager.export_(object);
	_dbus.objectManager.set_connection(_dbus.connection);
	_dbus.ownId = Gio::bus_own_name_on_connection(
		_dbus.connection,
		"org.mpris.MediaPlayer2." + (KSandbox::isFlatpak()
			? qEnvironmentVariable("FLATPAK_ID").toStdString()
			: KSandbox::isSnap()
			? qEnvironmentVariable("SNAP_NAME").toStdString()
			: QCoreApplication::applicationName().toStdString()),
		Gio::BusNameOwnerFlags::NONE_);
}

void SystemMediaControls::Private::deinit() {
	if (_dbus.ownId) {
		Gio::bus_unown_name(_dbus.ownId);
		_dbus.ownId = 0;
	}
	_dbus.objectManager = {};
}

SystemMediaControls::SystemMediaControls()
: _private(std::make_unique<Private>()) {
}

SystemMediaControls::~SystemMediaControls() {
	_private->deinit();
}

bool SystemMediaControls::init() {
	clearMetadata();

	return _private->dbusAvailable();
}

void SystemMediaControls::setApplicationName(const QString &name) {
	_private->set_identity(name.toStdString());
}

void SystemMediaControls::setEnabled(bool enabled) {
	if (enabled) {
		_private->init();
	} else {
		_private->deinit();
	}
}

void SystemMediaControls::setIsNextEnabled(bool value) {
	_private->player().set_can_go_next(value);
}

void SystemMediaControls::setIsPreviousEnabled(bool value) {
	_private->player().set_can_go_previous(value);
}

void SystemMediaControls::setIsPlayPauseEnabled(bool value) {
	_private->player().set_can_play(value);
	_private->player().set_can_pause(value);
}

void SystemMediaControls::setIsStopEnabled(bool value) {
}

void SystemMediaControls::setPlaybackStatus(PlaybackStatus status) {
	_private->player().set_playback_status(ConvertPlaybackStatus(status));
}

void SystemMediaControls::setLoopStatus(LoopStatus status) {
	// prevent property update -> rpl event -> property update recursion
	const auto statusString = ConvertLoopStatus(status);
	if (_private->player().get_loop_status() != statusString) {
		_private->player().set_loop_status(ConvertLoopStatus(status));
	}
}

void SystemMediaControls::setShuffle(bool value) {
	// prevent property update -> rpl event -> property update recursion
	if (_private->player().get_shuffle() != value) {
		_private->player().set_shuffle(value);
	}
}

void SystemMediaControls::setTitle(const QString &title) {
	auto metadata = GLib::VariantDict::new_(
		_private->player().get_metadata());
	metadata.insert_value("xesam:title", Q2V(title));
	_private->player().set_metadata(metadata.end());
}

void SystemMediaControls::setArtist(const QString &artist) {
	auto metadata = GLib::VariantDict::new_(
		_private->player().get_metadata());
	metadata.insert_value("xesam:artist", Q2V(artist));
	_private->player().set_metadata(metadata.end());
}

void SystemMediaControls::setThumbnail(const QImage &thumbnail) {
	QByteArray thumbnailData;
	QBuffer thumbnailBuffer(&thumbnailData);
	thumbnail.save(&thumbnailBuffer, "JPG", 87);

	auto metadata = GLib::VariantDict::new_(
		_private->player().get_metadata());
	metadata.insert_value(
		"mpris:artUrl",
		Q2V("data:image/jpeg;base64," + thumbnailData.toBase64()));
	_private->player().set_metadata(metadata.end());
}

void SystemMediaControls::setDuration(int duration) {
	_private->playerData().duration = duration * 1000;
	auto metadata = GLib::VariantDict::new_(
		_private->player().get_metadata());
	metadata.insert_value(
		"mpris:length",
		GLib::Variant::new_int64(_private->playerData().duration));
	_private->player().set_metadata(metadata.end());
}

void SystemMediaControls::setPosition(int position) {
	// get_position reads the value directly from variable
	// in the generated C code and doesn't account for the
	// property override. It's possible to override get_position
	// but the C++ bindings don't let do this due to
	// https://gitlab.gnome.org/GNOME/gobject-introspection/-/issues/183
	//
	// Once this is fixed, Player should be able to override get_position
	// and it will be possible to use (get|set)_position here
	// like other functions do with properties.
	auto prop = _private->player().property_position();
	const auto was = prop.get();
	prop.set(position * 1000);
	const auto playerPosition = prop.get();

	const auto positionDifference = was - playerPosition;
	if (positionDifference > 1000000 || positionDifference < -1000000) {
		_private->player().emit_seeked(playerPosition);
	}
}

void SystemMediaControls::setVolume(float64 volume) {
	// prevent property update -> rpl event -> property update recursion
	if (_private->player().get_volume() != volume) {
		_private->player().set_volume(volume);
	}
}

void SystemMediaControls::clearThumbnail() {
	auto metadata = GLib::VariantDict::new_(
		_private->player().get_metadata());
	metadata.remove("mpris:artUrl");
	_private->player().set_metadata(metadata.end());
}

void SystemMediaControls::clearMetadata() {
	const auto metadata = std::array{
		GLib::Variant::new_dict_entry(
			GLib::Variant::new_string("mpris:trackid"),
			// fake path
			GLib::Variant::new_variant(
				GLib::Variant::new_object_path("/org/desktop_app/track/0"))),
	};

	_private->player().set_metadata(
		GLib::Variant::new_array(metadata.data(), metadata.size()));
}

void SystemMediaControls::updateDisplay() {
}

auto SystemMediaControls::commandRequests() const
-> rpl::producer<SystemMediaControls::Command> {
	return _private->commandRequests();
}

rpl::producer<float64> SystemMediaControls::seekRequests() const {
	return _private->seekRequests(
	) | rpl::map([=](int64 position) {
		return float64(position) / _private->playerData().duration;
	});
}

rpl::producer<float64> SystemMediaControls::volumeChangeRequests() const {
	return _private->volumeChangeRequests();
}

rpl::producer<> SystemMediaControls::updatePositionRequests() const {
	return _private->updatePositionRequests();
}

bool SystemMediaControls::seekingSupported() const {
	return true;
}

bool SystemMediaControls::volumeSupported() const {
	return true;
}

bool SystemMediaControls::Supported() {
	return true;
}

} // namespace base::Platform
