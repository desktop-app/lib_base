#include "base/screen_reader_state.h"

#ifdef Q_OS_MAC
#include "base/platform/mac/base_screen_reader_state_mac.h"
#elif defined(Q_OS_WIN) // Q_OS_MAC
#include "base/platform/win/base_screen_reader_state_win.h"
#endif // Q_OS_MAC || Q_OS_WIN

namespace base {

ScreenReaderState::~ScreenReaderState() = default;

ScreenReaderState *ScreenReaderState::Instance() {
#ifdef Q_OS_MAC
	static auto instance = Platform::MacScreenReaderState();
#elif defined(Q_OS_WIN) // Q_OS_MAC
	static auto instance = Platform::WinScreenReaderState();
#else // Q_OS_MAC || Q_OS_WIN
	static auto instance = GeneralScreenReaderState();
#endif // Q_OS_MAC || Q_OS_WIN
	return &instance;
}

#if !defined(Q_OS_MAC) && !defined(Q_OS_WIN)

GeneralScreenReaderState::GeneralScreenReaderState()
: _isActive(QAccessible::isActive()) {
	QAccessible::installActivationObserver(this);
}

GeneralScreenReaderState::~GeneralScreenReaderState() {
	QAccessible::removeActivationObserver(this);
}

void GeneralScreenReaderState::accessibilityActiveChanged(bool active) {
	_isActive = active;
}

bool GeneralScreenReaderState::active() const {
	return _isActive.current();
}

rpl::producer<bool> GeneralScreenReaderState::activeValue() const {
	return _isActive.value();
}

#endif // !Q_OS_MAC && !Q_OS_WIN

} // namespace base
