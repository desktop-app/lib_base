
#pragma once

#include <QAccessible>
#include <QString>

#include "base/basic_types.h" 

class QWidget;

namespace base::Platform::Accessibility {

	void InstallFactory();
	void SetRole(not_null<QWidget*> widget, QAccessible::Role role);
	void SetName(not_null<QWidget*> widget, const QString& name);
	void Announce(not_null<QWidget*> widget, QAccessible::Event event);
} // namespace base::Platform::Accessibility
namespace Accessibility = base::Platform::Accessibility;