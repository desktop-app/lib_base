// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/integration.h"

#include <rpl/rpl.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QPointer>

namespace base {

// This method allows to create an rpl::producer from a Qt object
// and a signal with none or one reported value.
//
// QtSignalProducer(qtWindow, &QWindow::activeChanged) | rpl::start_
//
// This producer values construct a custom event loop leave point.
// This means that all postponeCall's will be invoked right after
// the value processing by the current consumer finishes.
template <typename Object, typename Signal>
auto qt_signal_producer(Object *object, Signal signal);

namespace details {

template <typename Signal>
struct qt_signal_argument;

template <
	typename Class,
	typename Return,
	typename Value1,
	typename Value2,
	typename ...Other>
struct qt_signal_argument<Return(Class::*)(Value1, Value2, Other...)> {
	using type = std::tuple<Value1, Value2, Other...>;
	static constexpr bool none = false;
	static constexpr bool one = false;
};

template <typename Class, typename Return, typename Value>
struct qt_signal_argument<Return(Class::*)(Value)> {
	using type = Value;
	static constexpr bool none = false;
	static constexpr bool one = true;
};

template <typename Class, typename Return>
struct qt_signal_argument<Return(Class::*)()> {
	using type = void;
	static constexpr bool none = true;
	static constexpr bool one = false;
};

} // namespace details

template <typename Object, typename Signal>
auto qt_signal_producer(Object *object, Signal signal) {
	using Value = typename details::qt_signal_argument<Signal>::type;
	static constexpr auto None = details::qt_signal_argument<Signal>::none;
	static constexpr auto One = details::qt_signal_argument<Signal>::one;
	using Produced = std::conditional_t<
		None,
		rpl::empty_value,
		std::remove_const_t<std::decay_t<Value>>>;
	const auto guarded = QPointer<Object>(object);
	return rpl::make_producer<Produced>([=](auto consumer) {
		if (!guarded) {
			return rpl::lifetime();
		}
		const auto connect = [&](auto &&handler) {
			const auto listener = new QObject(guarded.data());
			QObject::connect(
				guarded,
				signal,
				listener,
				std::forward<decltype(handler)>(handler));
			const auto weak = QPointer<QObject>(listener);
			return rpl::lifetime([=] {
				if (weak) {
					delete weak;
				}
			});
		};
		auto put = [=](const Produced &value) {
			if (QCoreApplication::instance()) {
				Integration::Instance().enterFromEventLoop([&] {
					consumer.put_next_copy(value);
				});
			} else {
				consumer.put_next_copy(value);
			}
		};
		if constexpr (None) {
			return connect([put = std::move(put)] { put({}); });
		} else if constexpr (One) {
			return connect(std::move(put));
		} else {
			return connect([put = std::move(put)](auto &&...args) {
				put(std::make_tuple(std::forward<decltype(args)>(args)...));
			});
		}
	});
}

} // namespace base
