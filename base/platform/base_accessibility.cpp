#include "base/platform/base_accessibility.h"
#include <QThread>
#include <QTimer>
#include <QAccessibleWidget>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#endif

#ifdef Q_OS_MAC
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef Q_OS_LINUX
#include <QProcess>
#endif

namespace base::Platform::Accessibility {

	void ScreenReaderDetector::performCheck() {
		const auto isActive = detectScreenReader();
		Q_EMIT resultReady(isActive);
	}

	bool ScreenReaderDetector::detectScreenReader() {
#ifdef Q_OS_WIN
		BOOL screenReaderRunning = FALSE;
		if (SystemParametersInfoW(SPI_GETSCREENREADER, 0, &screenReaderRunning, 0) && screenReaderRunning) {
			return true;
		}
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (snapshot != INVALID_HANDLE_VALUE) {
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
							CloseHandle(snapshot);
							return true;
						}
					}
				} while (Process32NextW(snapshot, &entry));
			}
			CloseHandle(snapshot);
		}
		return false;
#elif defined(Q_OS_MAC)
		Boolean keyExists = false;
		Boolean voiceOverEnabled = CFPreferencesGetAppBooleanValue(
			CFSTR("voiceOverOnOffKey"),
			CFSTR("com.apple.universalaccess"),
			&keyExists
		);
		return (keyExists && voiceOverEnabled);
#elif defined(Q_OS_LINUX)
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
		return QAccessible::isActive();
#endif
	}

	namespace {
		constexpr auto kRoleProperty = "_a11y_custom_role";
		constexpr auto kCheckIntervalMs = 2000;

		bool IsScreenReaderActive() {
			return ScreenReaderState::instance()->isActive();
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
		};

		QAccessibleInterface* Factory(const QString&, QObject* object) {
			if (object && object->isWidgetType()) {
				const auto widget = static_cast<QWidget*>(object);
				if (widget->property(kRoleProperty).isValid()) {
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

	void ObserveScreenReaderState(
		not_null<QWidget*> widget,
		std::function<void(bool)> callback) {
		callback(IsScreenReaderActive());

		auto* state = ScreenReaderState::instance();
		QObject::connect(
			state,
			&ScreenReaderState::stateChanged,
			widget, std::move(callback));
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