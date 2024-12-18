// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/platform/linux/base_linux_xcb_utilities.h"

namespace base::Platform::XCB {

class XSettings {
public:
	[[nodiscard]] static std::shared_ptr<XSettings> Instance();
	[[nodiscard]] bool initialized() const;

	[[nodiscard]] QVariant setting(const QByteArray &property) const;

	using PropertyChangeFunc = Fn<void(
		xcb_connection_t *connection,
		const QByteArray &name,
		const QVariant &property)>;

	[[nodiscard]] rpl::lifetime registerCallbackForProperty(
		const QByteArray &property,
		PropertyChangeFunc func);

private:
	XSettings();
	~XSettings();

	enum class Type {
		Integer,
		String,
		Color,
	};

	class PropertyValue;

	class Private;
	const std::unique_ptr<Private> _private;
};

} // namespace base::Platform::XCB
