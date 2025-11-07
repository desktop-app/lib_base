#pragma once

#include "base/basic_types.h"

#include <QAccessible>

namespace base {

class ScreenReaderState final : public QAccessible::ActivationObserver {
public:
	~ScreenReaderState();
	ScreenReaderState(const ScreenReaderState &) = delete;
	ScreenReaderState &operator=(const ScreenReaderState &) = delete;

	[[nodiscard]] static ScreenReaderState* Instance();
	[[nodiscard]] bool active() const;
	[[nodiscard]] rpl::producer<bool> activeValue() const;

private:
	ScreenReaderState();

	void accessibilityActiveChanged(bool active) override;

	rpl::variable<bool> _isActive;

};

} // namespace base
