// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/network_reachability.h"

#include "base/platform/base_platform_network_reachability.h"
#include "base/qt_signal_producer.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
#include <QtNetwork/QNetworkInformation>
#elif QT_VERSION < QT_VERSION_CHECK(6, 0, 0) // Qt >= 6.2.0
#include <QtNetwork/QNetworkConfigurationManager>
#endif // Qt >= 6.2.0 || Qt < 6.0.0

namespace base {
namespace {

std::weak_ptr<NetworkReachability> GlobalNetworkReachability;

} // namespace

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined __clang__ // __GNUC__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined _MSC_VER // __GNUC__ || __clang__
#pragma warning(push)
#pragma warning(disable:4996)
#endif // __GNUC__ || __clang__ || _MSC_VER

struct NetworkReachability::Private {
	Private() : platformHelper(Platform::NetworkReachability::Create()) {
	}

	rpl::variable<bool> available = true;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	QNetworkConfigurationManager configurationManager;
#endif // Qt < 6.0.0
	std::unique_ptr<Platform::NetworkReachability> platformHelper;
	rpl::lifetime lifetime;
};

NetworkReachability::NetworkReachability()
: _private(std::make_unique<Private>()) {
	if (_private->platformHelper) {
		_private->platformHelper->availableValue(
		) | rpl::start_with_next([=](bool available) {
			_private->available = available;
		}, _private->lifetime);
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
	} else if (QNetworkInformation::load(
		QNetworkInformation::Feature::Reachability)) {
		_private->available = QNetworkInformation::instance()->reachability()
			== QNetworkInformation::Reachability::Online;
		base::qt_signal_producer(
			QNetworkInformation::instance(),
			&QNetworkInformation::reachabilityChanged
		) | rpl::start_with_next([=](
				QNetworkInformation::Reachability newReachability) {
			_private->available = newReachability
				== QNetworkInformation::Reachability::Online;
		}, _private->lifetime);
#elif QT_VERSION < QT_VERSION_CHECK(6, 0, 0) // Qt >= 6.2.0
	} else {
		_private->available = _private->configurationManager.isOnline();
		QObject::connect(
			&_private->configurationManager,
			&QNetworkConfigurationManager::onlineStateChanged,
			[=](bool isOnline) {
				_private->available = isOnline;
			});
#endif // Qt >= 6.2.0 || Qt < 6.0.0
	}
}

NetworkReachability::~NetworkReachability() {
}

std::shared_ptr<NetworkReachability> NetworkReachability::Instance() {
	auto result = GlobalNetworkReachability.lock();
	if (!result) {
		GlobalNetworkReachability = result = std::make_shared<NetworkReachability>();
	}
	return result;
}

bool NetworkReachability::available() const {
	return _private->available.current();
}

rpl::producer<bool> NetworkReachability::availableChanges() const {
	return _private->available.changes();
}

rpl::producer<bool> NetworkReachability::availableValue() const {
	return _private->available.value();
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#elif defined __clang__ // __GNUC__
#pragma clang diagnostic pop
#elif defined _MSC_VER // __GNUC__ || __clang__
#pragma warning(pop)
#endif // __GNUC__ || __clang__ || _MSC_VER

} // namespace base
