# Copyright (C) 2015 Opera Software ASA.  All rights reserved.
#
# This file is an original work developed by Opera Software ASA

{
  'targets': [
    {
      'target_name': 'fraud_protection_ui',
      'type': 'static_library',
      'include_dirs': [
        '../..',
        '<(DEPTH)',
      ],
      'dependencies': [
        '<(DEPTH)/skia/skia.gyp:skia',
        '../resources/resources.gyp:resources',
        '../strings/strings.gyp:strings',
      ],
      'sources': [
        'fraud_protection.cc',
        'fraud_protection.h',
        'fraud_protection_delegate.h',
        'fraud_protection_delegate.cc',
        'fraud_warning_page.cc',
        'fraud_warning_page.h',
      ],
    },
  ],
}
