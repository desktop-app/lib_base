#include "base/screen_reader_state.h"

namespace base {

ScreenReaderState *ScreenReaderState::Instance() {
	static auto instance = ScreenReaderState();
	return &instance;
}

ScreenReaderState::ScreenReaderState()
: _isActive(QAccessible::isActive()) {
	QAccessible::installActivationObserver(this);
}

ScreenReaderState::~ScreenReaderState() {
	QAccessible::removeActivationObserver(this);
}

void ScreenReaderState::accessibilityActiveChanged(bool active) {
	_isActive = active;
}

bool ScreenReaderState::active() const {
	return _isActive.current();
}

rpl::producer<bool> ScreenReaderState::activeValue() const {
	return _isActive.value();
}

} // namespace base