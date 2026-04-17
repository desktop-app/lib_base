// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/screen_reader_state.h"
#include "base/weak_ptr.h"

namespace base::Platform {

class LinuxScreenReaderState final
	: public ScreenReaderState
	, public base::has_weak_ptr {
public:
	LinuxScreenReaderState();
	~LinuxScreenReaderState() override;

	bool active() const override;
	rpl::producer<bool> activeValue() const override;
	void updateActive();

private:
	struct Private;
	std::unique_ptr<Private> _private;
	rpl::variable<bool> _isActive;

};

} // namespace base::Platform
