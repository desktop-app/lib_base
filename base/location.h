// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

namespace base {

class Location {
public:
	struct Result {
		float64 latitude = 0.;
		float64 longitude = 0.;
		std::optional<float64> accuracy;

		[[nodiscard]] bool operator==(const Result &other) const {
			return latitude == other.latitude
				&& longitude == other.longitude
				&& accuracy == other.accuracy;
		}

		[[nodiscard]] bool operator!=(const Result &other) const {
			return !(*this == other);
		}
	};

	Location();
	~Location();

	[[nodiscard]] rpl::producer<std::optional<Result>> current();

private:
	struct Private;
	const std::unique_ptr<Private> _private;
};

} // namespace base
