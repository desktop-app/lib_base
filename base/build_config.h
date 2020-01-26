// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include <stdint.h>

// thanks Chromium

#if defined(__APPLE__)
#define OS_MAC 1
#elif defined(__linux__) // __APPLE__
#define OS_LINUX 1
#elif defined(_WIN32) // __APPLE__ || __linux__
#define OS_WIN 1
#else // __APPLE__ || __linux__ || _WIN32
#error Please add support for your platform in base/build_config.h
#endif // else for __APPLE__ || __linux__ || _WIN32

// For access to standard POSIXish features, use OS_POSIX instead of a
// more specific macro.
#if defined(OS_MAC) || defined(OS_LINUX)
#define OS_POSIX 1
#endif // OS_MAC || OS_LINUX

// Compiler detection.
#if defined(__clang__)
#define COMPILER_CLANG 1
#elif defined(__GNUC__) // __clang__
#define COMPILER_GCC 1
#elif defined(_MSC_VER) // __clang__ || __GNUC__
#define COMPILER_MSVC 1
#else // _MSC_VER || __clang__ || __GNUC__
#error Please add support for your compiler in base/build_config.h
#endif // else for _MSC_VER || __clang__ || __GNUC__

// Processor architecture detection.
#if defined(_M_X64) || defined(__x86_64__)
#define ARCH_CPU_X86_FAMILY 1
#define ARCH_CPU_X86_64 1
#define ARCH_CPU_64_BITS 1
#elif defined(_M_IX86) || defined(__i386__)
#define ARCH_CPU_X86_FAMILY 1
#define ARCH_CPU_X86 1
#define ARCH_CPU_32_BITS 1
#elif defined(__aarch64__)
#define ARCH_CPU_64_BITS 1
#elif defined(_M_ARM) || defined(__arm__)
#define ARCH_CPU_32_BITS 1
#elif defined(__ppc64__) || defined(__PPC64__)
#define ARCH_CPU_64_BITS 1
#elif defined(__e2k__)
#define ARCH_CPU_64_BITS 1
#else
#error Please add support for your architecture in base/build_config.h
#endif

#if defined(__GNUC__)
#define TG_FORCE_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define TG_FORCE_INLINE __forceinline
#else
#define TG_FORCE_INLINE inline
#endif

#include <climits>
static_assert(CHAR_BIT == 8, "Not supported char size.");
