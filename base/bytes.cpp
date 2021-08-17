// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/bytes.h"

#include "base/openssl_help.h"

namespace bytes {

void set_random(span destination) {
	if (!destination.empty()) {
		RAND_bytes(
			reinterpret_cast<unsigned char*>(destination.data()),
			destination.size());
	}
}

} // namespace bytes
