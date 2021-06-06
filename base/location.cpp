// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/location.h"

#include "base/const_string.h"
#include "base/platform/base_platform_location.h"
#include "base/platform/base_platform_cellular_utilities.h"
#include "base/platform/base_platform_wifi_utilities.h"
#include "base/platform/base_platform_bt_utilities.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

namespace base {
namespace {

constexpr auto kLocationServiceUrl = "https://location.services.mozilla.com/v1/geolocate?key=%1"_cs;
constexpr auto kApiKey = "geoclue"_cs;

QVariantList FormatCellNetworks() {
	QVariantList result;
	for (const auto network : Platform::CellNetworks()) {
		QVariantMap converted;
		if (!network.type.has_value()) {
			continue;
		}
		switch (*network.type) {
		case Platform::CellNetwork::Type::GSM_2G:
			converted["radioType"] = "gsm";
			break;
		case Platform::CellNetwork::Type::GSM_3G:
			converted["radioType"] = "wcdma";
			break;
		case Platform::CellNetwork::Type::GSM_4G:
			converted["radioType"] = "lte";
			break;
		default:
			continue;
		}
		if (!network.mcc.has_value()) {
			continue;
		}
		converted["mobileCountryCode"] = *network.mcc;
		if (!network.mnc.has_value()) {
			continue;
		}
		converted["mobileNetworkCode"] = *network.mnc;
		if (network.lac.has_value()) {
			converted["locationAreaCode"] = *network.lac;
		} else if (*network.type == Platform::CellNetwork::Type::GSM_2G) {
			continue;
		}
		if (network.rssi.has_value()) {
			converted["signalStrength"] = *network.rssi;
		}
		if (network.age.has_value()) {
			converted["age"] = *network.age;
		}
		result << converted;
	}
	return result;
}

QVariantList FormatWiFiNetworks() {
	QVariantList result;
	for (const auto network : Platform::WiFiNetworks()) {
		QVariantMap converted;
		if (!network.bssid.has_value()) {
			continue;
		}
		converted["macAddress"] = *network.bssid;
		if (network.age.has_value()) {
			converted["age"] = *network.age;
		}
		if (network.channel.has_value()) {
			converted["channel"] = *network.channel;
		}
		if (network.frequency.has_value()) {
			converted["frequency"] = *network.frequency;
		}
		if (network.rssi.has_value()) {
			converted["signalStrength"] = *network.rssi;
		}
		if (network.snr.has_value()) {
			converted["signalToNoiseRatio"] = *network.snr;
		}
		result << converted;
	}
	return result;
}

QVariantList FormatBTDevices() {
	QVariantList result;
	for (const auto network : Platform::BTDevices()) {
		QVariantMap converted;
		if (!network.mac.has_value()) {
			continue;
		}
		converted["macAddress"] = *network.mac;
		if (network.name.has_value()) {
			converted["name"] = *network.name;
		}
		if (network.age.has_value()) {
			converted["age"] = *network.age;
		}
		if (network.rssi.has_value()) {
			converted["signalStrength"] = *network.rssi;
		}
		result << converted;
	}
	return result;
}

QByteArray FormatRequest() {
	return QJsonDocument(QJsonObject::fromVariantMap({
		{ "considerIp", false },
		{ "cellTowers", FormatCellNetworks() },
		{ "wifiAccessPoints", FormatWiFiNetworks() },
		{ "bluetoothBeacons", FormatBTDevices() },
	})).toJson();
}

std::optional<Location::Result> ParseReply(const QByteArray &reply) {
	const auto object = QJsonDocument::fromJson(reply).object();
	if (!object.contains("location") || !object.contains("accuracy")) {
		return std::nullopt;
	}

	const auto location = object["location"].toObject();
	if (!location.contains("lat") || !location.contains("lng")) {
		return std::nullopt;
	}

	return Location::Result{
		location["lat"].toDouble(),
		location["lng"].toDouble(),
		object["accuracy"].toDouble(),
	};
}

} // namespace

struct Location::Private {
	QNetworkAccessManager networkAccessManager;
	rpl::variable<std::optional<Result>> current;
	bool finished = false;
};

Location::Location()
: _private(std::make_unique<Private>()) {
	if (const auto platformResult = Platform::Location()) {
		_private->current = platformResult;
		_private->finished = true;
		return;
	}

	QObject::connect(
		&_private->networkAccessManager,
		&QNetworkAccessManager::finished,
		[=](QNetworkReply *reply) {
			_private->current = ParseReply(reply->readAll());
			_private->finished = true;
		});

	auto request = QNetworkRequest(kLocationServiceUrl.utf16().arg(kApiKey.utf16()));
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	_private->networkAccessManager.post(request, FormatRequest());
}

Location::~Location() {
}

rpl::producer<std::optional<Location::Result>> Location::current() {
	return _private->finished
		? rpl::producer<std::optional<Result>>(_private->current.value())
		: rpl::producer<std::optional<Result>>(_private->current.changes());
}

} // namespace base
