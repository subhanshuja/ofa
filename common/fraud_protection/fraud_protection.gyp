# Copyright (C) 2015 Opera Software ASA.  All rights reserved.
#
# This file is an original work developed by Opera Software ASA

{
  'targets': [
    {
      'target_name': 'fraud_protection',
      'type': 'static_library',
      'include_dirs': [
        '../..',
        '<(DEPTH)',
      ],
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/third_party/icu/icu.gyp:icui18n',
        '<(DEPTH)/third_party/icu/icu.gyp:icuuc',
        '<(DEPTH)/third_party/libxml/libxml.gyp:libxml',
        '../constants/constants.gyp:opera_constants',
        '../net/common_net.gyp:opera_common_net',
      ],
      'sources': [
        'fraud_advisory.cc',
        'fraud_advisory.h',
        'fraud_protection_service.cc',
        'fraud_protection_service.h',
        'fraud_rated_server.cc',
        'fraud_rated_server.h',
        'fraud_url_rating.cc',
        'fraud_url_rating.h',
      ],
    },
  ],
}
