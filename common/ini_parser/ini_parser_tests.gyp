# Copyright (C) 2012 Opera Software ASA.  All rights reserved.
#
# This file is an original work developed by Opera Software ASA
{
  'targets':
  [
    {
      'target_name': 'ini_parser_unittests',
      'type': 'executable',
      'variables': {
        'chromium_code': 1,
      },
      'dependencies':
      [
        '<(DEPTH)/base/base.gyp:run_all_unittests', #Entry point for the test exe
        '<(DEPTH)/testing/gmock.gyp:gmock',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        'ini_parser.gyp:ini_parser'
      ],
      'include_dirs':
      [
        '../..',
        '<(DEPTH)',
      ],
      'sources':
      [
        'ini_parser_test.cc',
      ],
    }
  ],
}
