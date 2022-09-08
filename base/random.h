// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include <cstddef>
#include <type_traits>

#include "base/bytes.h"

namespace base {

void RandomFill(bytes::span bytes);

inline void RandomFill(void *data, std::size_t length) {
	RandomFill({ static_cast<bytes::type*>(data), length });
}

template <
	typename T,
	typename = std::enable_if_t<std::is_trivially_copyable_v<T>>>
[[nodiscard]] inline T RandomValue() {
	auto result = T();
	RandomFill({ reinterpret_cast<std::byte*>(&result), sizeof(T) });
	return result;
}

[[nodiscard]] int RandomIndex(int count);

template <typename T>
class BufferedRandom final {
public:
	explicit BufferedRandom(int bufferSize) : _buffer(bufferSize) {
		Expects(bufferSize > 0);
	}

	[[nodiscard]] T next() {
		if (_index == _buffer.size()) {
			_index = 0;
		}
		if (!_index) {
			RandomFill(bytes::make_span(_buffer));
		}
		return _buffer[_index++];
	}

private:
	std::vector<T> _buffer;
	int _index = 0;

};

[[nodiscard]] int RandomIndex(int count, BufferedRandom<uint32> buffered);

void RandomAddSeed(bytes::const_span bytes);

} // namespace base
