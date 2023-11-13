// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include <Unknwn.h>

#ifdef min
#undef min
#endif // min

#ifdef max
#undef max
#endif // max

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) < (b) ? (b) : (a))

#include <gdiplus.h>

#undef min
#undef max
