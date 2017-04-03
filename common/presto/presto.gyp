# Copyright (C) 2012 Opera Software ASA.  All rights reserved.
# This file is an original work developed by Opera Software ASA
{
  'variables':
  {
    'chromium_code': 1,  # Use higher warning level.
  },
  'targets':
  [
    {
      'target_name': 'presto',
      'type': 'static_library',
      'dependencies':
      [
        'password_crypto/password_crypto.gyp:password_crypto'
      ],
      'include_dirs':
      [
        '<(DEPTH)',
        '.'
      ],
      'sources':
      [

      ],
    },
    {
      'target_name': 'presto_unittests',
      'type': 'executable',
      'dependencies':
      [
        '<(DEPTH)/base/base.gyp:run_all_unittests', #Entry point for the test exe
        '<(DEPTH)/testing/gmock.gyp:gmock',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        'password_crypto/password_crypto.gyp:password_crypto_unittests',
      ],
      'sources':
      [
      ],
    }
  ],
}
