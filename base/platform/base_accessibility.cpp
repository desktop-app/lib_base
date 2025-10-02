#include "base/platform/base_accessibility.h"
#include <QThread>
#include <QTimer>
#include <QAccessibleWidget>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#elif defined(Q_OS_MAC)
#include <CoreFoundation/CoreFoundation.h>
#else // For Linux, BSD, and other Unix-like systems
#include <QProcess>
#endif

namespace base::Platform::Accessibility {

	void ScreenReaderDetector::performCheck() {
		const auto isActive = detectScreenReader();
		Q_EMIT resultReady(isActive);
	}

	bool ScreenReaderDetector::isScreenReaderActiveViaApi() {
#ifdef Q_OS_WIN
		BOOL screenReaderRunning = FALSE;
		return (SystemParametersInfoW(SPI_GETSCREENREADER, 0, &screenReaderRunning, 0) && screenReaderRunning);
#else
		return false;
#endif
	}

	bool ScreenReaderDetector::isScreenReaderProcessRunning() {
#ifdef Q_OS_WIN
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (snapshot == INVALID_HANDLE_VALUE) {
			return false;
		}

		struct HandleCloser {
			HANDLE handle;
			~HandleCloser() { CloseHandle(handle); }
		} closer = { snapshot };

		PROCESSENTRY32W entry;
		entry.dwSize = sizeof(PROCESSENTRY32W);
		const WCHAR* screenReaders[] = {
			L"nvda.exe", L"jaws.exe", L"jfw.exe",
			L"windowseyes.exe", L"narrator.exe", L"dragon.exe"
		};

		if (Process32FirstW(snapshot, &entry)) {
			do {
				for (const auto& reader : screenReaders) {
					if (_wcsicmp(entry.szExeFile, reader) == 0) {
						return true;
					}
				}
			} while (Process32NextW(snapshot, &entry));
		}
		return false;
#else
		return false;
#endif
	}

	bool ScreenReaderDetector::detectForWindows() {
		return isScreenReaderActiveViaApi() || isScreenReaderProcessRunning();
	}

	bool ScreenReaderDetector::detectForMac() {
#ifdef Q_OS_MAC
		Boolean keyExists = false;
		Boolean voiceOverEnabled = CFPreferencesGetAppBooleanValue(
			CFSTR("voiceOverOnOffKey"),
			CFSTR("com.apple.universalaccess"),
			&keyExists
		);
		return (keyExists && voiceOverEnabled);
#else
		return false;
#endif
	}

	bool ScreenReaderDetector::detectForLinux() {
#if !defined Q_OS_WIN && !defined Q_OS_MAC
		QStringList envVars = { "AT_SPI_BUS", "AT_SPI_IOR", "GNOME_ACCESSIBILITY" };
		for (const auto& var : envVars) {
			if (!qgetenv(var.toLocal8Bit().constData()).isEmpty()) {
				return true;
			}
		}
		QStringList screenReaders = { "orca", "speakup", "brltty" };
		for (const auto& reader : screenReaders) {
			QProcess process;
			process.start("pgrep", QStringList() << reader);
			if (process.waitForFinished(1000) && process.exitCode() == 0) {
				return true;
			}
		}
		return false;
#else
		return false;
#endif
	}

	bool ScreenReaderDetector::detectScreenReader() {
#ifdef Q_OS_WIN
		return detectForWindows();
#elif defined(Q_OS_MAC)
		return detectForMac();
#else
		return detectForLinux();
#endif
	}

	namespace {
		constexpr auto kRoleProperty = "_a11y_custom_role";
		constexpr auto kNameProperty = "_a11y_custom_name";
		constexpr auto kCheckIntervalMs = 1000;

		void SetupFocusManagementIfNeeded(not_null<QWidget*> widget) {
			const auto property = widget->property(kRoleProperty);
			if (!property.isValid()) {
				return;
			}
			const auto role = static_cast<QAccessible::Role>(property.toInt());

			if (role != QAccessible::Role::PushButton
				&& role != QAccessible::Role::Link
				&& role != QAccessible::Role::CheckBox
				&& role != QAccessible::Role::RadioButton) {
				return;
			}

			const auto updatePolicy = [widget](bool screenReaderIsActive) {
				widget->setFocusPolicy(screenReaderIsActive ? Qt::StrongFocus : Qt::NoFocus);
				};

			updatePolicy(ScreenReaderState::instance()->isActive());

			QObject::connect(
				ScreenReaderState::instance(),
				&ScreenReaderState::stateChanged,
				widget,
				updatePolicy
			);
		}

		class CustomAccessibilityInterface final : public QAccessibleWidget {
		public:
			using QAccessibleWidget::QAccessibleWidget;
			QAccessible::Role role() const override {
				const auto property = widget()->property(kRoleProperty);
				return property.isValid()
					? static_cast<QAccessible::Role>(property.toInt())
					: QAccessibleWidget::role();
			}

			QString text(QAccessible::Text t) const override {
				if (t == QAccessible::Name) {
					const auto property = widget()->property(kNameProperty);
					if (property.isValid()) {
						return property.toString();
					}
				}
				return QAccessibleWidget::text(t);
			}

			QAccessible::State state() const override {
				return QAccessibleWidget::state();
			}
		};

		QAccessibleInterface* Factory(const QString&, QObject* object) {
			if (object && object->isWidgetType()) {
				const auto widget = static_cast<QWidget*>(object);
				if (widget->property(kRoleProperty).isValid() || widget->property(kNameProperty).isValid()) {
					SetupFocusManagementIfNeeded(widget);
					return new CustomAccessibilityInterface(widget);
				}
			}
			return nullptr;
		}
	} // namespace

	void InstallFactory() {
		QAccessible::installFactory(Factory);
		ScreenReaderState::instance(); // اولین نمونه را می‌سازد و سیستم را فعال می‌کند.
	}

	void SetRole(not_null<QWidget*> widget, QAccessible::Role role) {
		widget->setProperty(kRoleProperty, static_cast<int>(role));
	}

	void SetName(not_null<QWidget*> widget, const QString& name) {
		widget->setProperty(kNameProperty, name);
	}

	ScreenReaderState* ScreenReaderState::instance() {
		static ScreenReaderState state;
		return &state;
	}

	ScreenReaderState::ScreenReaderState()
		: m_workerThread(new QThread(this))
		, m_detector(new ScreenReaderDetector) {
		m_detector->moveToThread(m_workerThread);

		m_timer = new QTimer(this);
		m_timer->setInterval(kCheckIntervalMs);

		connect(m_workerThread, &QThread::finished, m_detector, &QObject::deleteLater);
		connect(m_timer, &QTimer::timeout, m_detector, &ScreenReaderDetector::performCheck);
		connect(m_detector, &ScreenReaderDetector::resultReady, this, &ScreenReaderState::handleResult);

		m_workerThread->start();
		m_timer->start();

		QTimer::singleShot(0, m_detector, &ScreenReaderDetector::performCheck);
	}

	ScreenReaderState::~ScreenReaderState() {
		m_workerThread->quit();
		m_workerThread->wait();
	}

	bool ScreenReaderState::isActive() const {
		return m_isActive;
	}

	void ScreenReaderState::handleResult(bool isActive) {
		if (m_isActive != isActive) {
			m_isActive = isActive;
			Q_EMIT stateChanged(m_isActive);
		}
	}

} // namespace base::Platform::Accessibility