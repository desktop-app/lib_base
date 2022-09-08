// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/random.h"

extern "C" {
#include <openssl/rand.h>
} // extern "C"

namespace base {
namespace {

template <typename Next>
int RandomIndex(int count, Next &&next) {
	Expects(count > 0);

	if (count == 1) {
		return 0;
	}
	const auto max = (std::numeric_limits<uint32>::max() / count) * count;
	while (true) {
		const auto random = next();
		if (random < max) {
			return int(random % count);
		}
	}
}

} // namespace

void RandomFill(bytes::span bytes) {
	const auto result = RAND_bytes(
		reinterpret_cast<unsigned char*>(bytes.data()),
		bytes.size());

	Ensures(result);
}

int RandomIndex(int count) {
	return RandomIndex(count, [] { return  RandomValue<uint32>(); });
}

int RandomIndex(int count, BufferedRandom<uint32> buffered) {
	return RandomIndex(count, [&] { return buffered.next(); });
}

void RandomAddSeed(bytes::const_span bytes) {
	RAND_seed(bytes.data(), bytes.size());
}

} // namespace base
