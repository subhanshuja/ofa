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
      'target_name': 'password_crypto',
      'type': 'static_library',
      'dependencies':
      [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/third_party/boringssl/boringssl.gyp:boringssl'
      ],
      'include_dirs':
      [
        '<(DEPTH)/.',
        '.',
      ],
      'sources':
      [
        'crypto/openssl_des/des_locl.h',
        'crypto/openssl_des/des.h',
        'crypto/openssl_des/spr.h',
        'crypto/migration_crypto_symmetric_algorithm3DES.cc',
        'crypto/migration_crypto_symmetric_algorithm3DES.h',
        'crypto/migration_encryption_cbc.cc',
        'crypto/migration_encryption_cbc.h',
        # because boringssl doesn't export DES_encrypt3 / DES_decrypt3
        'crypto/openssl_des/des_enc.cc',
        'migration_master_password.cc',
        'migration_master_password.h',
        'migration_password_encryption.cc',
        'migration_password_encryption.h',
      ],
    },
    {
      'target_name': 'password_crypto_unittests',
      'type': 'executable',
      'dependencies':
      [
        '<(DEPTH)/base/base.gyp:run_all_unittests', #Entry point for the test exe
        '<(DEPTH)/testing/gmock.gyp:gmock',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        'password_crypto',

      ],
      'include_dirs':
      [
        '<(DEPTH)/.',
        '.',
      ],
      'sources':
      [
        'crypto/migration_crypto_test.cc',
        'migration_master_password_test.cc',
        'migration_password_encryption_test.cc',
      ],
    }
  ],
}
