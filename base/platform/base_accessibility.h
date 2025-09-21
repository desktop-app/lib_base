
#pragma once

#include <QAccessible>
#include <QString>

#include "base/basic_types.h" 

class QWidget;

namespace base::Platform::Accessibility {

	void InstallFactory();
	void SetRole(not_null<QWidget*> widget, QAccessible::Role role);
} // namespace base::Platform::Accessibility
namespace Accessibility = base::Platform::Accessibility;