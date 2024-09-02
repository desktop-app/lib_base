# This file is part of Desktop App Toolkit,
# a set of libraries for developing nice desktop applications.
#
# For license and copyright information please follow this link:
# https://github.com/desktop-app/legal/blob/master/LEGAL

add_library(lib_base STATIC)
add_library(desktop-app::lib_base ALIAS lib_base)
init_target(lib_base)

add_library(lib_base_crash_report_writer STATIC)
add_library(desktop-app::lib_base_crash_report_writer ALIAS lib_base_crash_report_writer)
init_target(lib_base_crash_report_writer)

get_filename_component(src_loc . REALPATH)

target_precompile_headers(lib_base PRIVATE $<$<COMPILE_LANGUAGE:CXX,OBJCXX>:${src_loc}/base/base_pch.h>)
nice_target_sources(lib_base ${src_loc}
PRIVATE
    base/platform/linux/base_battery_saving_linux.cpp
    base/platform/linux/base_battery_saving_linux.h
    base/platform/linux/base_file_utilities_linux.cpp
    base/platform/linux/base_file_utilities_linux.h
    base/platform/linux/base_global_shortcuts_linux.cpp
    base/platform/linux/base_global_shortcuts_linux.h
    base/platform/linux/base_haptic_linux.cpp
    base/platform/linux/base_haptic_linux.h
    base/platform/linux/base_info_linux.cpp
    base/platform/linux/base_info_linux.h
    base/platform/linux/base_last_input_linux.cpp
    base/platform/linux/base_last_input_linux.h
    base/platform/linux/base_layout_switch_linux.cpp
    base/platform/linux/base_layout_switch_linux.h
    base/platform/linux/base_linux_allocation_tracer.cpp
    base/platform/linux/base_linux_allocation_tracer.h
    base/platform/linux/base_linux_dbus_utilities.cpp
    base/platform/linux/base_linux_dbus_utilities.h
    base/platform/linux/base_linux_library.cpp
    base/platform/linux/base_linux_library.h
    base/platform/linux/base_linux_xcb_utilities.cpp
    base/platform/linux/base_linux_xcb_utilities.h
    base/platform/linux/base_linux_xdg_activation_token.cpp
    base/platform/linux/base_linux_xdg_activation_token.h
    base/platform/linux/base_linux_xdp_utilities.cpp
    base/platform/linux/base_linux_xdp_utilities.h
    base/platform/linux/base_linux_xsettings.cpp
    base/platform/linux/base_linux_xsettings.h
    base/platform/linux/base_network_reachability_linux.cpp
    base/platform/linux/base_power_save_blocker_linux.cpp
    base/platform/linux/base_power_save_blocker_linux.h
    base/platform/linux/base_process_linux.cpp
    base/platform/linux/base_process_linux.h
    base/platform/linux/base_system_media_controls_linux.cpp
    base/platform/linux/base_system_unlock_linux.cpp
    base/platform/linux/base_system_unlock_linux.h
    base/platform/linux/base_url_scheme_linux.cpp
    base/platform/linux/base_url_scheme_linux.h
    base/platform/mac/base_battery_saving_mac.mm
    base/platform/mac/base_battery_saving_mac.h
    base/platform/mac/base_confirm_quit.h
    base/platform/mac/base_confirm_quit.mm
    base/platform/mac/base_custom_app_icon_mac.h
    base/platform/mac/base_custom_app_icon_mac.mm
    base/platform/mac/base_file_utilities_mac.h
    base/platform/mac/base_file_utilities_mac.mm
    base/platform/mac/base_global_shortcuts_mac.mm
    base/platform/mac/base_global_shortcuts_mac.h
    base/platform/mac/base_haptic_mac.h
    base/platform/mac/base_haptic_mac.mm
    base/platform/mac/base_info_mac.h
    base/platform/mac/base_info_mac.mm
    base/platform/mac/base_last_input_mac.h
    base/platform/mac/base_last_input_mac.mm
    base/platform/mac/base_layout_switch_mac.h
    base/platform/mac/base_layout_switch_mac.mm
    base/platform/mac/base_network_reachability_mac.mm
    base/platform/mac/base_power_save_blocker_mac.h
    base/platform/mac/base_power_save_blocker_mac.mm
    base/platform/mac/base_process_mac.h
    base/platform/mac/base_process_mac.mm
    base/platform/mac/base_system_media_controls_mac.mm
    base/platform/mac/base_system_unlock_mac.h
    base/platform/mac/base_system_unlock_mac.mm
    base/platform/mac/base_url_scheme_mac.h
    base/platform/mac/base_url_scheme_mac.mm
    base/platform/mac/base_utilities_mac.h
    base/platform/mac/base_utilities_mac.mm
    base/platform/win/base_battery_saving_win.cpp
    base/platform/win/base_battery_saving_win.h
    base/platform/win/base_file_utilities_win.cpp
    base/platform/win/base_file_utilities_win.h
    base/platform/win/base_global_shortcuts_win.cpp
    base/platform/win/base_global_shortcuts_win.h
    base/platform/win/base_haptic_win.cpp
    base/platform/win/base_haptic_win.h
    base/platform/win/base_info_win.cpp
    base/platform/win/base_info_win.h
    base/platform/win/base_last_input_win.cpp
    base/platform/win/base_last_input_win.h
    base/platform/win/base_layout_switch_win.cpp
    base/platform/win/base_layout_switch_win.h
    base/platform/win/base_network_reachability_win.cpp
    base/platform/win/base_power_save_blocker_win.cpp
    base/platform/win/base_power_save_blocker_win.h
    base/platform/win/base_process_win.cpp
    base/platform/win/base_process_win.h
    base/platform/win/base_system_media_controls_win.cpp
    base/platform/win/base_system_unlock_win.cpp
    base/platform/win/base_system_unlock_win.h
    base/platform/win/base_url_scheme_win.cpp
    base/platform/win/base_url_scheme_win.h
    base/platform/win/base_windows_co_task_mem.h
    base/platform/win/base_windows_gdiplus_h.h
    base/platform/win/base_windows_rpcndr_h.h
    base/platform/win/base_windows_safe_library.cpp
    base/platform/win/base_windows_safe_library.h
    base/platform/win/base_windows_shlobj_h.h
    base/platform/win/base_windows_winrt.cpp
    base/platform/win/base_windows_winrt.h
    base/platform/win/base_windows_wrl.cpp
    base/platform/win/base_windows_wrl.h
    base/platform/win/wrl/wrl_implements_h.h
    base/platform/win/wrl/wrl_module_h.h
    base/platform/base_platform_custom_app_icon.h
    base/platform/base_platform_global_shortcuts.h
    base/platform/base_platform_haptic.h
    base/platform/base_platform_info.cpp
    base/platform/base_platform_info.h
    base/platform/base_platform_last_input.h
    base/platform/base_platform_layout_switch.h
    base/platform/base_platform_network_reachability.h
    base/platform/base_platform_file_utilities.h
    base/platform/base_platform_power_save_blocker.h
    base/platform/base_platform_process.h
    base/platform/base_platform_url_scheme.h
    base/platform/base_platform_system_media_controls.h
    base/algorithm.cpp
    base/algorithm.h
    base/assertion.cpp
    base/assertion.h
    base/atomic.h
    base/base_file_utilities.cpp
    base/base_file_utilities.h
    base/basic_types.h
    base/battery_saving.cpp
    base/battery_saving.h
    base/binary_guard.h
    base/build_config.h
    base/bytes.cpp
    base/bytes.h
    base/call_delayed.cpp
    base/call_delayed.h
    base/crc32hash.cpp
    base/crc32hash.h
    base/concurrent_timer.cpp
    base/concurrent_timer.h
    base/const_string.h
    base/custom_app_icon.h
    base/custom_delete.h
    base/debug_destroy_informer.h
    base/debug_log.cpp
    base/debug_log.h
    base/enum_mask.h
    base/event_filter.cpp
    base/event_filter.h
    base/expected.h
    base/file_lock.h
    base/file_lock_win.cpp
    base/file_lock_posix.cpp
    base/flags.h
    base/flat_map.h
    base/flat_set.h
    base/functors.h
    base/global_shortcuts.h
    base/global_shortcuts_generic.cpp
    base/global_shortcuts_generic.h
    base/index_based_iterator.h
    base/integration.cpp
    base/integration.h
    base/invoke_queued.h
    base/last_used_cache.h
    base/last_user_input.cpp
    base/last_user_input.h
    base/match_method.h
    base/network_reachability.cpp
    base/network_reachability.h
    base/never_freed_pointer.h
    base/object_ptr.h
    base/ordered_set.h
    base/openssl_help.h
    base/optional.h
    base/options.cpp
    base/options.h
    base/overload.h
    base/parse_helper.cpp
    base/parse_helper.h
    base/power_save_blocker.cpp
    base/power_save_blocker.h
    base/qthelp_regex.h
    base/qthelp_url.cpp
    base/qthelp_url.h
    base/qt_connection.h
    base/qt_signal_producer.h
    base/random.cpp
    base/random.h
    base/required.h
    base/runtime_composer.cpp
    base/runtime_composer.h
    base/single_instance.cpp
    base/single_instance.h
    base/system_unlock.h
    base/thread_safe_wrap.h
    base/timer.cpp
    base/timer.h
    base/timer_rpl.h
    base/type_traits.h
    base/unique_any.h
    base/unique_function.h
    base/unique_qptr.h
    base/unixtime.cpp
    base/unixtime.h
    base/variant.h
    base/virtual_method.h
    base/weak_ptr.h
    base/zlib_help.h
    base/qt/qt_common_adapters.h
    base/qt/qt_compare.h
    base/qt/qt_key_modifiers.h
    base/qt/qt_string_view.h
    base/qt/qt_tab_key.cpp
    base/qt/qt_tab_key.h

    base/base_pch.h
)

if (DESKTOP_APP_DISABLE_X11_INTEGRATION)
    remove_target_sources(lib_base ${src_loc}
        base/platform/linux/base_linux_xcb_utilities.cpp
        base/platform/linux/base_linux_xcb_utilities.h
        base/platform/linux/base_linux_xsettings.cpp
        base/platform/linux/base_linux_xsettings.h
    )
endif()

target_include_directories(lib_base
PUBLIC
    ${src_loc}
)

target_link_libraries(lib_base
PUBLIC
    desktop-app::lib_rpl
    desktop-app::lib_crl
    desktop-app::external_qt
    desktop-app::external_openssl
    desktop-app::external_crash_reports
    desktop-app::external_ranges
    desktop-app::external_gsl
    desktop-app::external_expected
PRIVATE
    desktop-app::external_xxhash
)

if (WIN32)
    nuget_add_winrt(lib_base)
elseif (LINUX)
    target_link_libraries(lib_base
    PUBLIC
        desktop-app::external_kcoreaddons
    )

    include(${cmake_helpers_loc}/external/glib/generate_dbus.cmake)
    generate_dbus(lib_base org.freedesktop. XdgDBus ${src_loc}/base/platform/linux/org.freedesktop.DBus.xml)
    generate_dbus(lib_base org.freedesktop.portal. XdpInhibit ${third_party_loc}/xdg-desktop-portal/data/org.freedesktop.portal.Inhibit.xml)
    generate_dbus(lib_base org.freedesktop.portal. XdpOpenURI ${third_party_loc}/xdg-desktop-portal/data/org.freedesktop.portal.OpenURI.xml)
    generate_dbus(lib_base org.freedesktop.portal. XdpRequest ${third_party_loc}/xdg-desktop-portal/data/org.freedesktop.portal.Request.xml)
    generate_dbus(lib_base org.freedesktop.portal. XdpSettings ${third_party_loc}/xdg-desktop-portal/data/org.freedesktop.portal.Settings.xml)
    generate_dbus(lib_base org.freedesktop. XdgFileManager1 ${src_loc}/base/platform/linux/org.freedesktop.FileManager1.xml)
    generate_dbus(lib_base org.gnome.Mutter. MutterIdleMonitor ${src_loc}/base/platform/linux/org.gnome.Mutter.IdleMonitor.xml)
    generate_dbus(lib_base org.mpris. Mpris ${src_loc}/base/platform/linux/mpris.xml)
    generate_dbus(lib_base io.snapcraft. Snapcraft ${src_loc}/base/platform/linux/io.snapcraft.Settings.xml)

    if (NOT DESKTOP_APP_DISABLE_X11_INTEGRATION)
        target_link_libraries(lib_base
        PUBLIC
            desktop-app::external_xcb_keysyms
            desktop-app::external_xcb_record
            desktop-app::external_xcb_screensaver
            desktop-app::external_xcb
        )
    endif()
endif()

if (DESKTOP_APP_USE_ALLOCATION_TRACER)
    target_compile_definitions(lib_base
    PRIVATE
        DESKTOP_APP_USE_ALLOCATION_TRACER
    )
endif()

#target_precompile_headers(lib_base_crash_report_writer REUSE_FROM lib_base)
target_precompile_headers(lib_base_crash_report_writer PRIVATE ${src_loc}/base/base_pch.h)
nice_target_sources(lib_base_crash_report_writer ${src_loc}
PRIVATE
    base/crash_report_header.cpp
    base/crash_report_header.h
    base/crash_report_writer.cpp
    base/crash_report_writer.h
)

if (DESKTOP_APP_DISABLE_CRASH_REPORTS)
    remove_target_sources(lib_base_crash_report_writer ${src_loc}
        base/crash_report_writer.cpp
        base/crash_report_writer.h
    )
endif()

target_link_libraries(lib_base_crash_report_writer PUBLIC desktop-app::lib_base)
