# This file is part of Desktop App Toolkit,
# a set of libraries for developing nice desktop applications.
#
# For license and copyright information please follow this link:
# https://github.com/desktop-app/legal/blob/master/LEGAL

{
  'includes': [
    '../gyp_helpers/common/common.gypi',
  ],
  'targets': [{
    'target_name': 'lib_base',
    'includes': [
      '../gyp_helpers/common/library.gypi',
      '../gyp_helpers/modules/openssl.gypi',
      '../gyp_helpers/modules/qt.gypi',
      '../gyp_helpers/modules/pch.gypi',
    ],
    'variables': {
      'src_loc': '.',
      'list_sources_command': 'python gyp/list_sources.py --input gyp/sources.txt --replace src_loc=<(src_loc)',
      'pch_source': '<(src_loc)/base/base_pch.cpp',
      'pch_header': '<(src_loc)/base/base_pch.h',
    },
    'defines': [
    ],
    'dependencies': [
      '../lib_crl/lib_crl.gyp:lib_crl',
    ],
    'export_dependent_settings': [
      '../lib_crl/lib_crl.gyp:lib_crl',
    ],
    'include_dirs': [
      '<(src_loc)',
      '<(libs_loc)/range-v3/include',
      '<(submodules_loc)/lib_rpl',
      '<(submodules_loc)/GSL/include',
      '<(submodules_loc)/variant/include',
    ],
    'direct_dependent_settings': {
      'include_dirs': [
        '<(src_loc)',
        '<(libs_loc)/range-v3/include',
        '<(submodules_loc)/lib_rpl',
        '<(submodules_loc)/GSL/include',
        '<(submodules_loc)/variant/include',
      ],
    },
    'sources': [
      'gyp/sources.txt',
      '<!@(<(list_sources_command) <(qt_moc_list_sources_arg))',
    ],
    'sources!': [
      '<!@(<(list_sources_command) <(qt_moc_list_sources_arg) --exclude_for <(build_os))',
    ],
    'conditions': [[ 'build_macold', {
      'xcode_settings': {
        'OTHER_CPLUSPLUSFLAGS': [ '-nostdinc++' ],
      },
      'include_dirs': [
        '/usr/local/macold/include/c++/v1',
      ],
    }]],
  }],
}
