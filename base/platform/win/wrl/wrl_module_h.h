// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/platform/win/base_windows_wrl.h"

#pragma warning(push)
// class has virtual functions, but destructor is not virtual
#pragma warning(disable:4265)
#pragma warning(disable:5104)
#include <wrl/module.h>
#pragma warning(pop)
