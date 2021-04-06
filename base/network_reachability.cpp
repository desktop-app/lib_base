// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/network_reachability.h"

#include "base/platform/base_platform_network_reachability.h"

#include <QtNetwork/QNetworkConfigurationManager>

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
	rpl::variable<bool> available;
	//QObject qObject;
	QNetworkConfigurationManager configurationManager;
	rpl::lifetime lifetime;
};

NetworkReachability::NetworkReachability()
: _private(std::make_unique<Private>()) {
	if (Platform::NetworkAvailableSupported()) {
		_private->available = *Platform::NetworkAvailable();
		Platform::NetworkAvailableChanged(
		) | rpl::start_with_next([=] {
			_private->available = *Platform::NetworkAvailable();
		}, _private->lifetime);
	/* Qt 6
	} else if (QNetworkInformation::load()) {
		_private->available = QNetworkInformation::instance()->reachability()
			!= QNetworkInformation::Reachability::Disconnected;
		QObject::connect(
			QNetworkInformation::instance(),
			&QNetworkInformation::reachabilityChanged,
			&_private->qObject,
			[=](QNetworkInformation::Reachability newReachability) {
				_private->available = newReachability
					!= QNetworkInformation::Reachability::Disconnected;
			});
	*/
	} else {
		_private->available = _private->configurationManager.isOnline();
		QObject::connect(
			&_private->configurationManager,
			&QNetworkConfigurationManager::onlineStateChanged,
			[=](bool isOnline) {
				_private->available = isOnline;
			});
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
