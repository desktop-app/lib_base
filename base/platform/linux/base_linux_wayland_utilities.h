// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include <wayland-client-core.h>

struct wl_registry;

namespace base::Platform::Wayland {

template <typename T>
class AutoDestroyer : public T {
public:
	using T::T;

	AutoDestroyer(const AutoDestroyer &other) = delete;
	AutoDestroyer &operator=(const AutoDestroyer &other) = delete;

	AutoDestroyer(AutoDestroyer &&other) {
		*this = std::move(other);
	}

	AutoDestroyer &operator=(AutoDestroyer &&other) {
		destroy();
		static_cast<T&>(*this) = other;
		other._moved = true;
		return *this;
	}

	~AutoDestroyer() {
		destroy();
	}

private:
	void destroy() {
		if (!this->isInitialized() || _moved) {
			return;
		}

		static constexpr auto HasDestroy = requires(T t) {
			t.destroy();
		};

		if constexpr (HasDestroy) {
			static_cast<T*>(this)->destroy();
		} else {
			wl_proxy_destroy(reinterpret_cast<wl_proxy*>(this->object()));
		}
	}

	bool _moved = false;
};

template <typename T>
class Global : public AutoDestroyer<T> {
public:
	Global(::wl_registry *registry, uint32_t id, int version)
	: AutoDestroyer<T>(registry, id, version)
	, _id(id) {
	}

	uint32_t id() const {
		return _id;
	}

private:
	uint32_t _id = 0;
};

} // namespace base::Platform::Wayland
