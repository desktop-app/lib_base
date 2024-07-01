// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/win/base_system_unlock_win.h"

#include "base/platform/win/base_windows_winrt.h"

#include <QtWidgets/QWidget>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Security.Credentials.UI.h>
#include <UserConsentVerifierInterop.h>

namespace base {
namespace {

using namespace winrt::Windows::Security::Credentials::UI;

using AsyncStatus = winrt::Windows::Foundation::AsyncStatus;
using winrt::Windows::Foundation::IAsyncOperation;

} // namespace

rpl::producer<SystemUnlockAvailability> SystemUnlockStatus(
		bool lookupDetails) {
	static auto result = rpl::variable<SystemUnlockAvailability>();
	WinRT::Try([&] {
		UserConsentVerifier::CheckAvailabilityAsync().Completed([](
				IAsyncOperation<UserConsentVerifierAvailability> that,
				AsyncStatus status) {
			const auto results = WinRT::Try([&] {
				return that.GetResults();
			});
			const auto available = (status == AsyncStatus::Completed)
				&& (results == UserConsentVerifierAvailability::Available);
			crl::on_main([=] {
				result = SystemUnlockAvailability{
					.known = true,
					.available = available,
				};
			});
		});
	});
	return result.value();
}

void SuggestSystemUnlock(
		not_null<QWidget*> parent,
		const QString &text,
		Fn<void(SystemUnlockResult)> done) {
	const auto window = parent->window();
	window->createWinId();
	const auto handle = reinterpret_cast<HWND>(window->winId());
	if (!handle) {
		done(SystemUnlockResult::Cancelled);
		return;
	}

	const auto completed = [=](
			IAsyncOperation<UserConsentVerificationResult> that,
			AsyncStatus status) -> void {
		const auto results = WinRT::Try([&] {
			return that.GetResults();
		});
		done((status != AsyncStatus::Completed)
			? SystemUnlockResult::Cancelled
			: (results == UserConsentVerificationResult::Verified)
			? SystemUnlockResult::Success
			: (results == UserConsentVerificationResult::RetriesExhausted)
			? SystemUnlockResult::FloodError
			: SystemUnlockResult::Cancelled);
	};
	const auto success = WinRT::Try([&] {
		const auto interop = winrt::get_activation_factory<
			UserConsentVerifier,
			IUserConsentVerifierInterop>();
		if (!interop) {
			done(SystemUnlockResult::Cancelled);
			return;
		}

		const auto wtext = text.toStdString();
		const auto htext = winrt::to_hstring(wtext);
		winrt::capture<IAsyncOperation<UserConsentVerificationResult>>(
			interop,
			&IUserConsentVerifierInterop::RequestVerificationForWindowAsync,
			handle,
			reinterpret_cast<HSTRING>(winrt::get_abi(htext))
		).Completed(completed);
	});
	if (!success) {
		done(SystemUnlockResult::Cancelled);
		return;
	}
}

} // namespace base
