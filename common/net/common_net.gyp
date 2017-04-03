# Copyright (C) 2013 Opera Software ASA.  All rights reserved.
# This file is an original work developed by Opera Software ASA

{
  'variables': {
    'chromium_code': 1,
  },
  'targets' : [
    {
      'target_name': 'opera_common_net',
      'type': 'static_library',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/net/net.gyp:net',
        '<(DEPTH)/ui/base/ui_base.gyp:ui_base',
        '../strings/strings.gyp:strings',
      ],
      'export_dependent_settings': [
        '<(DEPTH)/base/base.gyp:base',
      ],
      'include_dirs': [
        '<(DEPTH)',
        '../..',
      ],
      'sources': [
        'certificate_util.cc',
        'certificate_util_android.cc',
        'certificate_util_ios.cc',
        'certificate_util_linux.cc',
        'certificate_util_mac.mm',
        'certificate_util_win.cc',
        'certificate_util.h',
        'opera_cert_verifier_delegate.cc',
        'opera_cert_verifier_delegate.h',
        'sensitive_url_request_user_data.cc',
        'sensitive_url_request_user_data.h',
      ],
    },
    {
      'target_name': 'chromium_compatible_net_resource_provider',
      'type': 'static_library',
      'include_dirs': [
        '<(DEPTH)',
        '../..',
      ],
      'dependencies': [
        '<(DEPTH)/net/net.gyp:net_resources',
        '<(DEPTH)/ui/base/ui_base.gyp:ui_base',
        '../strings/strings.gyp:strings',
      ],
      'sources': [
        'net_resource_provider.cc',
        'net_resource_provider.h',
      ],
    },
  ],
  'conditions': [
    ['OS != "ios"', {
      'targets': [
        {
          'target_name': 'opera_common_net_unittests',
          'type': 'executable',
          'dependencies': [
            '<(DEPTH)/base/base.gyp:run_all_unittests',
            '<(DEPTH)/base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
            '<(DEPTH)/content/content_shell_and_tests.gyp:test_support_content',
            '<(DEPTH)/net/net.gyp:net_test_support',
            '<(DEPTH)/testing/gtest.gyp:gtest',
            'opera_common_net',
          ],
          'include_dirs': [
            '<(DEPTH)',
            '../..',
          ],
          'sources': [
            'opera_cert_verifier_delegate_unittest.cc',
          ],
          'conditions': [
            ['OS=="win" and win_use_allocator_shim==1', {
              'dependencies': [
                '<(DEPTH)/base/allocator/allocator.gyp:allocator',
              ],
            }],
          ],
        },
      ],
    }],
  ],
}
