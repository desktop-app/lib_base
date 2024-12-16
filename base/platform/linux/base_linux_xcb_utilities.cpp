// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/linux/base_linux_xcb_utilities.h"

#include "base/qt/qt_common_adapters.h"

#include <QtCore/QAbstractEventDispatcher>
#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QSocketNotifier>
#include <QtGui/QGuiApplication>

#if QT_VERSION < QT_VERSION_CHECK(6, 2, 0)
#include <qpa/qplatformnativeinterface.h>
#endif // Qt < 6.2.0

namespace base::Platform::XCB {
namespace {

std::weak_ptr<CustomConnection> GlobalCustomConnection;

class QtEventFilter : public QAbstractNativeEventFilter {
public:
	QtEventFilter(Fn<void(xcb_generic_event_t*)> handler)
	: _handler(handler) {
		QCoreApplication::instance()->installNativeEventFilter(this);
	}

private:
	bool nativeEventFilter(
			const QByteArray &eventType,
			void *message,
			native_event_filter_result *result) override {
		_handler(reinterpret_cast<xcb_generic_event_t*>(message));
		return false;
	}

	Fn<void(xcb_generic_event_t*)> _handler;
};

base::flat_map<
	xcb_connection_t*,
	std::pair<
		std::variant<
			v::null_t,
			std::unique_ptr<QSocketNotifier>,
			std::unique_ptr<QtEventFilter>
		>,
		std::vector<std::unique_ptr<Fn<void(xcb_generic_event_t*)>>>
	>
> EventHandlers;

} // namespace

std::shared_ptr<CustomConnection> SharedConnection() {
	auto result = GlobalCustomConnection.lock();
	if (!result) {
		GlobalCustomConnection = result = std::make_shared<CustomConnection>();
	}
	return result;
}

xcb_connection_t *GetConnectionFromQt() {
#if defined QT_FEATURE_xcb && QT_CONFIG(xcb)
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
	using namespace QNativeInterface;
	const auto native = qApp->nativeInterface<QX11Application>();
#else // Qt >= 6.2.0
	const auto native = QGuiApplication::platformNativeInterface();
#endif // Qt < 6.2.0
	if (!native) {
		return nullptr;
	}

#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
	return native->connection();
#else // Qt >= 6.2.0
	return reinterpret_cast<xcb_connection_t*>(
		native->nativeResourceForIntegration(QByteArray("connection")));
#endif // Qt < 6.2.0
#else // xcb
	return nullptr;
#endif // !xcb
}

rpl::lifetime InstallEventHandler(
		xcb_connection_t *connection,
		Fn<void(xcb_generic_event_t*)> handler) {
	if (!connection || xcb_connection_has_error(connection)) {
		return rpl::lifetime();
	}

	auto it = EventHandlers.find(connection);
	if (it == EventHandlers.cend()) {
		using EventHandlerVector = decltype(
			EventHandlers
		)::value_type::second_type::second_type;

		if (connection == GetConnectionFromQt()) {
			it = EventHandlers.emplace(
				connection,
				std::make_pair(
					std::make_unique<QtEventFilter>([=](
							xcb_generic_event_t *event) {
						const auto it = EventHandlers.find(connection);
						for (const auto &handler : it->second.second) {
							(*handler)(event);
						}
					}),
					EventHandlerVector()
				)
			).first;
		} else {
			it = EventHandlers.emplace(
				connection,
				std::make_pair(
					std::make_unique<QSocketNotifier>(
						xcb_get_file_descriptor(connection),
						QSocketNotifier::Read
					),
					EventHandlerVector()
				)
			).first;

			auto &notifier = *v::get<std::unique_ptr<QSocketNotifier>>(
				it->second.first);

			QObject::connect(
				QCoreApplication::eventDispatcher(),
				&QAbstractEventDispatcher::aboutToBlock,
				&notifier,
				[=] {
					const auto it = EventHandlers.find(connection);
					EventPointer<xcb_generic_event_t> event;
					while (!xcb_connection_has_error(connection)
							&& (event = MakeEventPointer(
								xcb_poll_for_event(connection)))) {
						for (const auto &handler : it->second.second) {
							(*handler)(event.get());
						}
					}
					// Let handlers handle the error
					if (xcb_connection_has_error(connection)) {
						for (const auto &handler : it->second.second) {
							(*handler)(nullptr);
						}
						it->second.first = v::null;
					}
				});

			notifier.setEnabled(true);
		}
	}

	const auto ptr = it->second.second.emplace_back(new Fn(handler)).get();
	return rpl::lifetime([=] {
		const auto it = EventHandlers.find(connection);
		it->second.second.erase(
			ranges::remove(
				it->second.second,
				ptr,
				&decltype(it->second.second)::value_type::get),
			it->second.second.end());
		if (it->second.second.empty()) {
			EventHandlers.remove(connection);
		}
	});
}

xcb_timestamp_t GetTimestamp(xcb_connection_t *connection) {
	if (!connection || xcb_connection_has_error(connection)) {
		return XCB_CURRENT_TIME;
	}

	const auto window = GetRootWindow(connection);
	if (!window) {
		return XCB_CURRENT_TIME;
	}

	const auto atom = GetAtom(connection, "_DESKTOP_APP_GET_TIMESTAMP");
	if (!atom) {
		return XCB_CURRENT_TIME;
	}

	const auto eventMask = ChangeWindowEventMask(
		connection,
		window,
		XCB_EVENT_MASK_PROPERTY_CHANGE);

	if (!eventMask) {
		return XCB_CURRENT_TIME;
	}

	QEventLoop loop;
	xcb_timestamp_t timestamp = XCB_CURRENT_TIME;
	const auto lifetime = InstallEventHandler(
		connection,
		[&](xcb_generic_event_t *event) {
			if (!event) {
				loop.quit();
				return;
			}

			const auto guard = gsl::finally([&] {
				free(
					xcb_get_input_focus_reply(
						connection,
						xcb_get_input_focus(connection),
						nullptr));
			});

			if ((event->response_type & ~0x80) != XCB_PROPERTY_NOTIFY) {
				return;
			}

			const auto pn = reinterpret_cast<xcb_property_notify_event_t*>(
				event);

			if (pn->window != window || pn->atom != atom) {
				return;
			}

			timestamp = pn->time;
			loop.quit();
		});

	if (!lifetime) {
		return XCB_CURRENT_TIME;
	}

	const auto error = MakeErrorPointer(
		xcb_request_check(
			connection,
			xcb_change_property_checked(
				connection,
				XCB_PROP_MODE_REPLACE,
				window,
				atom,
				XCB_ATOM_INTEGER,
				32,
				0,
				nullptr)));

	if (error) {
		return XCB_CURRENT_TIME;
	}

	loop.exec();
	return timestamp;
}

xcb_window_t GetRootWindow(xcb_connection_t *connection) {
	if (!connection || xcb_connection_has_error(connection)) {
		return XCB_NONE;
	}

	const auto screen = xcb_setup_roots_iterator(
		xcb_get_setup(connection)).data;

	if (!screen) {
		return XCB_NONE;
	}

	return screen->root;
}

xcb_atom_t GetAtom(xcb_connection_t *connection, const QString &name) {
	if (!connection || xcb_connection_has_error(connection)) {
		return XCB_NONE;
	}

	const auto cookie = xcb_intern_atom(
		connection,
		0,
		name.size(),
		name.toUtf8().constData());

	const auto reply = MakeReplyPointer(xcb_intern_atom_reply(
		connection,
		cookie,
		nullptr));

	if (!reply) {
		return XCB_NONE;
	}

	return reply->atom;
}

bool IsExtensionPresent(
		xcb_connection_t *connection,
		xcb_extension_t *ext) {
	if (!connection || xcb_connection_has_error(connection)) {
		return false;
	}

	const auto reply = xcb_get_extension_data(
		connection,
		ext);

	if (!reply) {
		return false;
	}

	return reply->present;
}

std::vector<xcb_atom_t> GetWMSupported(
		xcb_connection_t *connection,
		xcb_window_t root) {
	auto netWmAtoms = std::vector<xcb_atom_t>{};

	if (!connection || xcb_connection_has_error(connection)) {
		return netWmAtoms;
	}

	const auto supportedAtom = GetAtom(connection, "_NET_SUPPORTED");
	if (!supportedAtom) {
		return netWmAtoms;
	}

	auto offset = 0;
	auto remaining = 0;

	do {
		const auto cookie = xcb_get_property(
			connection,
			false,
			root,
			supportedAtom,
			XCB_ATOM_ATOM,
			offset,
			1024);

		const auto reply = MakeReplyPointer(xcb_get_property_reply(
			connection,
			cookie,
			nullptr));

		if (!reply) {
			break;
		}

		remaining = 0;

		if (reply->type == XCB_ATOM_ATOM && reply->format == 32) {
			const auto len = xcb_get_property_value_length(reply.get())
				/ sizeof(xcb_atom_t);

			const auto atoms = reinterpret_cast<xcb_atom_t*>(
				xcb_get_property_value(reply.get()));

			const auto s = netWmAtoms.size();
			netWmAtoms.resize(s + len);
			memcpy(netWmAtoms.data() + s, atoms, len * sizeof(xcb_atom_t));

			remaining = reply->bytes_after;
			offset += len;
		}
	} while (remaining > 0);

	return netWmAtoms;
}

xcb_window_t GetSupportingWMCheck(
		xcb_connection_t *connection,
		xcb_window_t root) {
	if (!connection || xcb_connection_has_error(connection)) {
		return XCB_NONE;
	}

	const auto supportingAtom = base::Platform::XCB::GetAtom(
		connection,
		"_NET_SUPPORTING_WM_CHECK");

	if (!supportingAtom) {
		return XCB_NONE;
	}

	const auto cookie = xcb_get_property(
		connection,
		false,
		root,
		supportingAtom,
		XCB_ATOM_WINDOW,
		0,
		1024);

	const auto reply = MakeReplyPointer(xcb_get_property_reply(
		connection,
		cookie,
		nullptr));

	if (!reply) {
		return XCB_NONE;
	}

	return (reply->format == 32 && reply->type == XCB_ATOM_WINDOW)
		? *reinterpret_cast<xcb_window_t*>(
			xcb_get_property_value(reply.get()))
		: XCB_NONE;
}

bool IsSupportedByWM(xcb_connection_t *connection, const QString &atomName) {
	if (!connection || xcb_connection_has_error(connection)) {
		return false;
	}

	const auto root = GetRootWindow(connection);
	if (!root) {
		return false;
	}

	const auto atom = GetAtom(connection, atomName);
	if (!atom) {
		return false;
	}

	return ranges::contains(GetWMSupported(connection, root), atom);
}

rpl::lifetime ChangeWindowEventMask(
		xcb_connection_t *connection,
		xcb_window_t window,
		uint mask,
		ChangeWindowEventMaskMode mode,
		bool revert) {
	using Mode = ChangeWindowEventMaskMode;
	if (!connection || xcb_connection_has_error(connection)) {
		return rpl::lifetime();
	}

	const auto windowAttribsCookie = xcb_get_window_attributes(
		connection,
		window);

	const auto windowAttribs = MakeReplyPointer(xcb_get_window_attributes_reply(
		connection,
		windowAttribsCookie,
		nullptr));
	
	const uint oldMask = windowAttribs ? windowAttribs->your_event_mask : 0;

	if ((mode == Mode::Add) && (oldMask & mask)) {
		return rpl::lifetime([] {});
	} else if ((mode == Mode::Remove) && !(oldMask & mask)) {
		return rpl::lifetime([] {});
	} else if (oldMask == mask) {
		return rpl::lifetime([] {});
	}

	const uint value[] = {
		mode == Mode::Add
			? oldMask | mask
			: mode == Mode::Remove
			? oldMask & ~mask
			: mask
	};

	const auto error = MakeErrorPointer(
		xcb_request_check(
			connection,
			xcb_change_window_attributes_checked(
				connection,
				window,
				XCB_CW_EVENT_MASK,
				value)));

	if (error) {
		return rpl::lifetime();
	}

	if (!revert) {
		return rpl::lifetime([] {});
	}

	return rpl::lifetime([=] {
		if (xcb_connection_has_error(connection)) {
			return;
		}

		const uint value[] = {
			oldMask
		};

		free(
			xcb_request_check(
				connection,
				xcb_change_window_attributes_checked(
					connection,
					window,
					XCB_CW_EVENT_MASK,
					value)));
	});
}

} // namespace base::Platform::XCB
