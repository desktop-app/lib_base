// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

namespace base {

template <auto fn>
struct custom_delete {
    template <typename T>
    constexpr void operator()(T* value) const {
        fn(value);
    }
};

} // namespace base
