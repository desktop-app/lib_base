#pragma once

#include "base/basic_types.h"

#include <QAccessible>

namespace base {

class ScreenReaderState : public QAccessible::ActivationObserver {
public:
	~ScreenReaderState();

	static ScreenReaderState* Instance();
	rpl::producer<bool> activeValue() const;

protected:
	void accessibilityActiveChanged(bool active) override;

private:
	ScreenReaderState();
	Q_DISABLE_COPY(ScreenReaderState)

	rpl::variable<bool> _isActive;

};

} // namespace base
