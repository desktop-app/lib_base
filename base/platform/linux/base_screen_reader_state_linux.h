// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/screen_reader_state.h"
#include "base/weak_ptr.h"

#include <QAccessible>

#include <memory>

namespace base::Platform {

class LinuxScreenReaderState final
	: public ScreenReaderState
	, public QAccessible::ActivationObserver
	, public base::has_weak_ptr {
public:
	LinuxScreenReaderState();
	~LinuxScreenReaderState() override;

	bool active() const override;
	rpl::producer<bool> activeValue() const override;

private:
	void accessibilityActiveChanged(bool active) override;
	void refreshStatusProxy();
	void updateActive();

	struct Private;
	const std::unique_ptr<Private> _private;
	const bool _useX11ScreenReaderDetection = false;
	rpl::variable<bool> _isActive;

};

} // namespace base::Platform
