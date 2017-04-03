# Copyright (C) 2012 Opera Software ASA.  All rights reserved.
# This file is an original work developed by Opera Software ASA

{
  'targets': [
    {
      'target_name': 'ini_parser',
      'type': 'static_library',
      'include_dirs': [
        '../..',
        '<(DEPTH)',
      ],
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
      ],
      'sources': [
        'ini_parser.cc',
        'ini_parser.h',
      ],
    },
  ], # 'targets'
}
