#pragma once

#include "base/basic_types.h"

#ifndef Q_OS_MAC
#include <QAccessible>
#endif

namespace base {

class ScreenReaderState {
public:
	virtual ~ScreenReaderState();
	ScreenReaderState(const ScreenReaderState &) = delete;
	ScreenReaderState &operator=(const ScreenReaderState &) = delete;

	[[nodiscard]] static ScreenReaderState *Instance();
	[[nodiscard]] virtual bool active() const = 0;
	[[nodiscard]] virtual rpl::producer<bool> activeValue() const = 0;

protected:
	ScreenReaderState() = default;

};

#ifndef Q_OS_MAC
class GeneralScreenReaderState final
	: public ScreenReaderState
	, public QAccessible::ActivationObserver {
public:
	GeneralScreenReaderState();
	~GeneralScreenReaderState() override;

	bool active() const override;
	rpl::producer<bool> activeValue() const override;

private:
	void accessibilityActiveChanged(bool active) override;

	rpl::variable<bool> _isActive;

};
#endif

} // namespace base
