// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include <wayland-client.h>

namespace base::Platform::Wayland {

struct RegistryDeleter {
	void operator()(wl_registry *value) {
		wl_registry_destroy(value);
	}
};

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

} // namespace base::Platform::Wayland
