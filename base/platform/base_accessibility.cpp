#include "base/platform/base_accessibility.h"

#include <QAccessibleWidget>
#include <QWidget>
#include <QAccessible>
#include <QCoreApplication>
#include <QAccessibleEvent>

namespace base::Platform::Accessibility {

    namespace {

        constexpr auto kRoleProperty = "_a11y_custom_role";
        constexpr auto kNameProperty = "_a11y_custom_name";

        class CustomAccessibilityInterface final : public QAccessibleWidget {
        public:
            using QAccessibleWidget::QAccessibleWidget;

            QAccessible::Role role() const override {
                const auto property = widget()->property(kRoleProperty);
                if (property.isValid()) {
                    return static_cast<QAccessible::Role>(property.toInt());
                }
                return QAccessibleWidget::role();
            }

            QString text(QAccessible::Text t) const override {
                if (t == QAccessible::Text::Name) {
                    const auto property = widget()->property(kNameProperty);
                    if (property.isValid()) {
                        return property.toString();
                    }
                }
                return QAccessibleWidget::text(t);
            }
        };

        QAccessibleInterface* Factory(const QString& classname, QObject* object) {
            if (object && object->isWidgetType()) {
                const auto widget = static_cast<QWidget*>(object);
                if (widget->property(kRoleProperty).isValid() || widget->property(kNameProperty).isValid()) {
                    return new CustomAccessibilityInterface(widget);
                }
            }
            return nullptr;
        }

    } // namespace

    void InstallFactory() {
        QAccessible::installFactory(Factory);
    }

    void SetRole(not_null<QWidget*> widget, QAccessible::Role role) {
        widget->setProperty(kRoleProperty, static_cast<int>(role));
    }

    void SetName(not_null<QWidget*> widget, const QString& name) {
        widget->setProperty(kNameProperty, name);
    }

    void Announce(not_null<QWidget*> widget, QAccessible::Event event) {
        QAccessibleEvent eventObject(widget.get(), event);
        QAccessible::updateAccessibility(&eventObject);
    }

} // namespace base::Platform::Accessibility