# Copyright (C) 2012 Opera Software ASA.  All rights reserved.
# This file is an original work developed by Opera Software ASA

{
  'variables':
  {
    'chromium_code': 1,  # Use higher warning level.
     # Turns off signature verification for resources like partner content,
     # browserjs, prefs override, etc.
    'disable_crypto_signature_check%': 0,
  },
  'targets':
  [
    {
      'target_name': 'opcrypto',
      'type': 'static_library',
      'dependencies':
      [
        '<(DEPTH)/crypto/crypto.gyp:crypto',
        '<(DEPTH)/third_party/modp_b64/modp_b64.gyp:modp_b64',
      ],
      'include_dirs':
      [
        '<(DEPTH)/.',
        '.',
        '../..',
      ],
      'sources':
      [
       'op_verify_signature.cc',
       'op_verify_signature.h',
      ],
      'conditions' : [
        ['disable_crypto_signature_check==1', {
          'defines' : [
            'DISABLE_CRYPTO_SIGNATURE_CHECK',
          ],
        }]
      ],
    },
    {
      'target_name': 'opcrypto_unittests',
      'type': 'executable',
      'dependencies':
      [
        'opcrypto.gyp:opcrypto',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/base.gyp:run_all_unittests', #Entry point for the test exe
        '<(DEPTH)/testing/gmock.gyp:gmock',
        '<(DEPTH)/testing/gtest.gyp:gtest',
      ],
      'include_dirs':
      [
        '<(DEPTH)/.',
        '.',
        '../..',
      ],
      'sources':
      [
        'op_verify_signature_unittest.cc',
      ],
      'copies':
      [{
        'destination':'<(PRODUCT_DIR)/',
        'files':
        [
          'crypto_test_data/'
        ]
      }]
    }
  ],
}
