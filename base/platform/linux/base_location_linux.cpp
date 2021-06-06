// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/base_platform_location.h"

#include "base/platform/linux/base_linux_library.h"

#include "../../../../ThirdParty/gpsd/include/gps.h"

namespace base::Platform {
namespace {

int (*gps_open)(const char *, const char *, struct gps_data_t *);
int (*gps_close)(struct gps_data_t *);
int (*gps_read)(struct gps_data_t *, char *message, int message_len);
bool (*gps_waiting)(const struct gps_data_t *, int);
int (*gps_stream)(struct gps_data_t *, unsigned int, void *);

bool LoadLibGPS() {
	static const auto Inited = [] {
		QLibrary lib;
		if (!LoadLibrary(lib, "gps")) return false;
		if (!LOAD_LIBRARY_SYMBOL(lib, gps_open)) return false;
		if (!LOAD_LIBRARY_SYMBOL(lib, gps_close)) return false;
		if (!LOAD_LIBRARY_SYMBOL(lib, gps_read)) return false;
		if (!LOAD_LIBRARY_SYMBOL(lib, gps_waiting)) return false;
		if (!LOAD_LIBRARY_SYMBOL(lib, gps_stream)) return false;
		return true;
	}();

	return Inited;
}

} // namespace

std::optional<Location::Result> Location() {
	if (!LoadLibGPS()) {
		return std::nullopt;
	}

	struct gps_data_t gpsData;

	if (gps_open("localhost", DEFAULT_GPSD_PORT, &gpsData) != 0) {
		return std::nullopt;
	}

	const auto guard = gsl::finally([&] {
		gps_stream(&gpsData, WATCH_DISABLE, nullptr);
		gps_close(&gpsData);
	});

	gps_stream(&gpsData, WATCH_ENABLE | WATCH_JSON, nullptr);

	while (gps_waiting(&gpsData, 5000000)) {
		if (gps_read(&gpsData, nullptr, 0) == -1) {
			break;
		}

		if ((MODE_SET & gpsData.set) != MODE_SET) {
			continue;
		}

		if (!std::isfinite(gpsData.fix.latitude)
			|| !std::isfinite(gpsData.fix.longitude)) {
			break;
		}

		return Location::Result{
			gpsData.fix.latitude,
			gpsData.fix.longitude,
		};
	}

	return std::nullopt;
}

} // namespace base::Platform
