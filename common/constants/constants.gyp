# Copyright (C) 2015 Opera Software ASA.  All rights reserved.
#
# This file is an original work developed by Opera Software ASA

{
  'targets': [
    {
      'target_name': 'opera_constants',
      'type': 'static_library',
      'include_dirs': [
        '../..',
      ],
      'sources': [
        'pref_names.cc',
        'pref_names.h',
        'switches.cc',
        'switches.h',
      ],
    },
    {
      'target_name': 'chrome_constants',
      'type': 'static_library',
      'include_dirs': [
        '<(DEPTH)',
      ],
      'sources': [
        '<(DEPTH)/chrome/common/chrome_constants.cc',
        '<(DEPTH)/chrome/common/chrome_constants.h',
        '<(DEPTH)/chrome/common/chrome_switches.cc',
        '<(DEPTH)/chrome/common/chrome_switches.h',
      ],
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/chrome/common_constants.gyp:version_header',
      ],
    },
  ],
}
