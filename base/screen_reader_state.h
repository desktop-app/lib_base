#pragma once

#include "base/basic_types.h"
#include "crl/crl.h"
#include "rpl/rpl.h"
#include <qaccessible.h>

namespace base {
	class ScreenReaderState : public QAccessible::ActivationObserver {
	public:
		static ScreenReaderState* Instance();
		rpl::producer<bool> activeValue() const;
		~ScreenReaderState();

	protected:
		void accessibilityActiveChanged(bool active) override;

	private:
		ScreenReaderState();
		Q_DISABLE_COPY(ScreenReaderState)

			rpl::variable<bool> _isActive;
	};
} // namespace base