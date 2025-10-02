#pragma once

#include "base/basic_types.h"
#include <QWidget>
#include <QAccessible>
#include <QObject>
#include <functional>

class QThread;
class QTimer;

namespace base {
	namespace Platform {
		namespace Accessibility {

			void InstallFactory();
			void SetRole(not_null<QWidget*> widget, QAccessible::Role role);
			void SetName(not_null<QWidget*> widget, const QString& name);

			class ScreenReaderDetector : public QObject {
				Q_OBJECT

			public Q_SLOTS:
				void performCheck();

			Q_SIGNALS:
				void resultReady(bool isActive);

			private:
				bool detectScreenReader();
				bool detectForWindows();
				bool isScreenReaderActiveViaApi();
				bool isScreenReaderProcessRunning();

				bool detectForMac();
				bool detectForLinux();
			};

			class ScreenReaderState : public QObject {
				Q_OBJECT

			public:
				static ScreenReaderState* instance();
				bool isActive() const;

			Q_SIGNALS:
				void stateChanged(bool active);

			private Q_SLOTS:
				void handleResult(bool isActive);

			private:
				ScreenReaderState();
				~ScreenReaderState();
				Q_DISABLE_COPY(ScreenReaderState)

					bool m_isActive = false;
				QTimer* m_timer;
				QThread* m_workerThread;
				ScreenReaderDetector* m_detector;
			};

		} // namespace Accessibility
	} // namespace Platform
} // namespace base

namespace Accessibility = base::Platform::Accessibility;