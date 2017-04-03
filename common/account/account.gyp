# -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
#
# Copyright (C) 2013 Opera Software ASA.  All rights reserved.
#
# This file is an original work developed by Opera Software ASA

{
  'variables': {
    'opera_root_dir': '../..',
    'opera_common_dir': '<(opera_root_dir)/common',
    'opera_desktop_dir': '<(opera_root_dir)/desktop',
  },
  'targets' : [
    {
      'target_name': 'account',
      'type': 'static_library',
      'dependencies': [
        '<(DEPTH)/net/net.gyp:net',
        '<(DEPTH)/url/url.gyp:url_lib',
        '<(opera_common_dir)/sync/sync.gyp:sync_config',
        'account_auth_data'
      ],
      'sources': [
        'account_observer.h',
        'account_service.cc',
        'account_service.h',
        'account_service_delegate.h',
        'account_service_impl.cc',
        'account_service_impl.h',
        'account_util.cc',
        'account_util.h',
        'oauth_token_fetcher.cc',
        'oauth_token_fetcher.h',
        'time_skew_resolver.h',
        'time_skew_resolver_impl.cc',
        'time_skew_resolver_impl.h',
      ],
      'include_dirs': [
        '<(DEPTH)',
        '../..',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../..',
        ],
      },
      'conditions' : [
        ['OS!="android" and OS!="ios"', {
          'dependencies': [
            '<(DEPTH)/google_apis/google_apis.gyp:google_apis',
            '../net/common_net.gyp:opera_common_net',
          ],
          'sources+': [
            'oauth_token_fetcher_impl.h',
            'oauth_token_fetcher_impl.cc',
          ],
        }],
        ['opera_desktop == 1', {
          'dependencies': [
            '<(opera_desktop_dir)/common/sync/sync.gyp:sync_config_impl',
          ],
        }],
      ],
    },
    {
      'target_name': 'account_auth_data',
      'type': 'static_library',
      'sources': [
        'account_auth_data.h'
      ],
      'include_dirs': [
        '<(DEPTH)',
        '../..',
      ],
    },
    {
      'target_name': 'test_support_account',
      'type': 'static_library',
      'dependencies': [
        '<(DEPTH)/testing/gmock.gyp:gmock',
        '<(DEPTH)/url/url.gyp:url_lib',
        'account',
      ],
      'sources': [
        'account_auth_data_fetcher_mock.cc',
        'account_auth_data_fetcher_mock.h',
        'mock_account_observer.cc',
        'mock_account_observer.h',
        'mock_account_service.cc',
        'mock_account_service.h',
        'time_skew_resolver_mock.cc',
        'time_skew_resolver_mock.h'
      ],
      'include_dirs': [
        '<(DEPTH)',
        '../..',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../..',
        ],
      },
    },
  ],
  'conditions': [
    ['OS != "ios"', {
      'targets': [
        {
          'target_name': 'account_unittests',
          'type': 'executable',
          'dependencies': [
            '<(DEPTH)/base/base.gyp:run_all_unittests',
            '<(DEPTH)/content/content_shell_and_tests.gyp:test_support_content',
            '<(DEPTH)/net/net.gyp:net_test_support',
            '<(DEPTH)/testing/gmock.gyp:gmock',
            '<(DEPTH)/testing/gtest.gyp:gtest',
            '<(opera_common_dir)/sync/sync.gyp:sync_test_support',
            'account',
            'test_support_account',
          ],
          'include_dirs': [
            '<(DEPTH)',
          ],
          'sources': [
            'account_service_impl_test.cc',
            'account_util_test.cc',
            'oauth_token_fetcher_test.cc',
            'time_skew_resolver_impl_test.cc',
          ],
        },
      ],
    }],
  ],
}
