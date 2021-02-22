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

target_precompile_headers(lib_base PRIVATE ${src_loc}/base/base_pch.h)
nice_target_sources(lib_base ${src_loc}
PRIVATE
    base/platform/linux/base_file_utilities_linux.cpp
    base/platform/linux/base_file_utilities_linux.h
    base/platform/linux/base_global_shortcuts_linux.cpp
    base/platform/linux/base_global_shortcuts_linux.h
    base/platform/linux/base_info_linux.cpp
    base/platform/linux/base_info_linux.h
    base/platform/linux/base_last_input_linux.cpp
    base/platform/linux/base_last_input_linux.h
    base/platform/linux/base_layout_switch_linux.cpp
    base/platform/linux/base_layout_switch_linux.h
    base/platform/linux/base_linux_glib_helper.h
    base/platform/linux/base_linux_gtk_integration_p.h
    base/platform/linux/base_linux_gtk_integration.cpp
    base/platform/linux/base_linux_gtk_integration.h
    base/platform/linux/base_linux_xcb_utilities.cpp
    base/platform/linux/base_linux_xcb_utilities.h
    base/platform/linux/base_linux_xlib_helper.cpp
    base/platform/linux/base_linux_xlib_helper.h
    base/platform/linux/base_process_linux.cpp
    base/platform/linux/base_process_linux.h
    base/platform/linux/base_url_scheme_linux.cpp
    base/platform/linux/base_url_scheme_linux.h
    base/platform/mac/base_file_utilities_mac.h
    base/platform/mac/base_file_utilities_mac.mm
    base/platform/mac/base_global_shortcuts_mac.mm
    base/platform/mac/base_global_shortcuts_mac.h
    base/platform/mac/base_info_mac.h
    base/platform/mac/base_info_mac.mm
    base/platform/mac/base_last_input_mac.h
    base/platform/mac/base_last_input_mac.mm
    base/platform/mac/base_layout_switch_mac.h
    base/platform/mac/base_layout_switch_mac.mm
    base/platform/mac/base_process_mac.h
    base/platform/mac/base_process_mac.mm
    base/platform/mac/base_url_scheme_mac.h
    base/platform/mac/base_url_scheme_mac.mm
    base/platform/mac/base_utilities_mac.h
    base/platform/mac/base_utilities_mac.mm
    base/platform/win/base_file_utilities_win.cpp
    base/platform/win/base_file_utilities_win.h
    base/platform/win/base_global_shortcuts_win.cpp
    base/platform/win/base_global_shortcuts_win.h
    base/platform/win/base_info_win.cpp
    base/platform/win/base_info_win.h
    base/platform/win/base_last_input_win.cpp
    base/platform/win/base_last_input_win.h
    base/platform/win/base_layout_switch_win.cpp
    base/platform/win/base_layout_switch_win.h
    base/platform/win/base_process_win.cpp
    base/platform/win/base_process_win.h
    base/platform/win/base_url_scheme_win.cpp
    base/platform/win/base_url_scheme_win.h
    base/platform/win/base_windows_h.h
    base/platform/win/base_windows_wrl.cpp
    base/platform/win/base_windows_wrl.h
    base/platform/win/base_windows_safe_library.cpp
    base/platform/win/base_windows_safe_library.h
    base/platform/win/wrl/wrl_implements_h.h
    base/platform/base_platform_global_shortcuts.h
    base/platform/base_platform_info.h
    base/platform/base_platform_last_input.h
    base/platform/base_platform_layout_switch.h
    base/platform/base_platform_file_utilities.h
    base/platform/base_platform_process.h
    base/platform/base_platform_url_scheme.h
    base/algorithm.h
    base/assertion.cpp
    base/assertion.h
    base/base_file_utilities.cpp
    base/base_file_utilities.h
    base/basic_types.h
    base/binary_guard.h
    base/build_config.h
    base/bytes.h
    base/call_delayed.cpp
    base/call_delayed.h
    base/crc32hash.cpp
    base/crc32hash.h
    base/concurrent_timer.cpp
    base/concurrent_timer.h
    base/const_string.h
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
    base/object_ptr.h
    base/observer.cpp
    base/observer.h
    base/ordered_set.h
    base/openssl_help.h
    base/optional.h
    base/overload.h
    base/parse_helper.cpp
    base/parse_helper.h
    base/qthelp_regex.h
    base/qthelp_url.cpp
    base/qthelp_url.h
    base/qt_connection.h
    base/qt_signal_producer.h
    base/required.h
    base/runtime_composer.cpp
    base/runtime_composer.h
    base/single_instance.cpp
    base/single_instance.h
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
    base/value_ordering.h
    base/variant.h
    base/virtual_method.h
    base/weak_ptr.h
    base/zlib_help.h

    base/base_pch.h
)

if (DESKTOP_APP_DISABLE_GTK_INTEGRATION)
    remove_target_sources(lib_base ${src_loc}
        base/platform/linux/base_linux_gtk_integration_p.h
        base/platform/linux/base_linux_gtk_integration.cpp
        base/platform/linux/base_linux_xlib_helper.cpp
        base/platform/linux/base_linux_xlib_helper.h
    )

    nice_target_sources(lib_base ${src_loc}
    PRIVATE
        base/platform/linux/base_linux_gtk_integration_dummy.cpp
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
)

if (LINUX)
    target_link_libraries(lib_base
    PUBLIC
        desktop-app::external_xcb_keysyms
        desktop-app::external_xcb_record
        desktop-app::external_xcb_screensaver
        desktop-app::external_xcb
        desktop-app::external_glib
    )

    if (NOT DESKTOP_APP_DISABLE_GTK_INTEGRATION)
        find_package(PkgConfig REQUIRED)

        if (DESKTOP_APP_USE_PACKAGED AND NOT DESKTOP_APP_USE_PACKAGED_LAZY)
            pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
            pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)

            target_link_libraries(lib_base
            PRIVATE
                PkgConfig::GTK3
                PkgConfig::X11
            )
        else()
            pkg_search_module(GTK REQUIRED gtk+-3.0 gtk+-2.0)
            target_include_directories(lib_base PRIVATE ${GTK_INCLUDE_DIRS})
            target_link_libraries(lib_base PRIVATE X11)
        endif()
    endif()
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

target_link_libraries(lib_base_crash_report_writer PUBLIC desktop-app::lib_base)
