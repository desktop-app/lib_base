// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/base_platform_location.h"

#include "base/platform/win/base_windows_h.h"
#include "base/platform/win/base_windows_safe_library.h"

#include <wrl/client.h>
#include <initguid.h>
#include <sensorsapi.h>
#include <sensors.h>

#pragma comment(lib, "sensorsapi.lib")

using namespace Microsoft::WRL;

namespace base::Platform {

std::optional<Location::Result> Location() {
	ComPtr<ISensorManager> sensorManager;
	HRESULT hr = CoCreateInstance(
		CLSID_SensorManager,
		nullptr,
		CLSCTX_ALL,
		IID_PPV_ARGS(&sensorManager));

	if (FAILED(hr)) {
		return std::nullopt;
	}

	ComPtr<ISensorCollection> sensorList;
	hr = sensorManager->GetSensorsByCategory(
		SENSOR_CATEGORY_LOCATION,
		&sensorList);

	if (FAILED(hr)) {
		return std::nullopt;
	}

	hr = sensorManager->RequestPermissions(0, sensorList.Get(), true);
	if (FAILED(hr)) {
		return std::nullopt;
	}

	ULONG sensorCount = 0;
	hr = sensorList->GetCount(&sensorCount);
	if (FAILED(hr)) {
		return std::nullopt;
	}

	for (auto i = 0; i != sensorCount; ++i) {
		ComPtr<ISensor> sensor;
		hr = sensorList->GetAt(i, &sensor);
		if (FAILED(hr)) {
			continue;
		}

		SensorState sensorState;
		hr = sensor->GetState(&sensorState);
		if (FAILED(hr)
			|| (sensorState != SENSOR_STATE_READY && sensorState != SENSOR_STATE_MIN)) {
			continue;
		}

		ComPtr<ISensorDataReport> sensorData;
		hr = sensor->GetData(&sensorData);
		if (FAILED(hr)) {
			continue;
		}

		PROPVARIANT latitude;
		hr = sensorData->GetSensorValue(
			SENSOR_DATA_TYPE_LATITUDE_DEGREES,
			&latitude);

		if (FAILED(hr)) {
			continue;
		}

		PROPVARIANT longitude;
		hr = sensorData->GetSensorValue(
			SENSOR_DATA_TYPE_LONGITUDE_DEGREES,
			&longitude);

		if (FAILED(hr)) {
			continue;
		}

		return Location::Result{
			latitude.dblVal,
			longitude.dblVal,
			[&]() -> std::optional<float64> {
				PROPVARIANT result;
				hr = sensorData->GetSensorValue(
					SENSOR_DATA_TYPE_ERROR_RADIUS_METERS,
					&result);

				if (FAILED(hr) || result.dblVal == 0) {
					return std::nullopt;
				}

				return result.dblVal;
			}(),
		};
	}

	return std::nullopt;
}

} // namespace base::Platform
