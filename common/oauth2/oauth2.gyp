# -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
#
# Copyright (C) 2016 Opera Software ASA.  All rights reserved.
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
      'target_name': 'oauth2',
      'type': 'static_library',
      'dependencies': [
        'oauth2_get_session_name',
        'oauth2_shared',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/components/components.gyp:os_crypt',
        '<(DEPTH)/components/components.gyp:webdata_common',
        '<(DEPTH)/net/net.gyp:net',
        '<(DEPTH)/url/url.gyp:url_lib',
        '<(opera_common_dir)/constants/constants.gyp:opera_constants',
        '<(opera_common_dir)/sync/sync.gyp:sync_config',
        '<(opera_common_dir)/sync/sync.gyp:sync_login_data'
      ],
      'sources': [
        'auth_service.h',
        'auth_service_factory.cc',
        'auth_service_factory.h',
        'auth_service_impl.cc',
        'auth_service_impl.h',
        'pref_names.cc',
        'pref_names.h',
        'client/auth_service_client.h',
        'diagnostics/diagnostic_service.cc',
        'device_name/device_name_service.h',
        'device_name/device_name_service_factory.cc',
        'device_name/device_name_service_factory.h',
        'device_name/device_name_service_impl.cc',
        'device_name/device_name_service_impl.h',
        'diagnostics/diagnostic_service.h',
        'diagnostics/diagnostic_service_factory.cc',
        'diagnostics/diagnostic_service_factory.h',
        'diagnostics/diagnostic_supplier.cc',
        'diagnostics/diagnostic_supplier.h',
        'migration/oauth1_migrator.h',
        'migration/oauth1_migrator_impl.cc',
        'migration/oauth1_migrator_impl.h',
        'migration/oauth1_session_data.cc',
        'migration/oauth1_session_data.h',
        'network/access_token_request.cc',
        'network/access_token_request.h',
        'network/access_token_response.cc',
        'network/access_token_response.h',
        'network/migration_token_request.cc',
        'network/migration_token_request.h',
        'network/migration_token_response.cc',
        'network/migration_token_response.h',
        'network/network_request.h',
        'network/network_request_manager.h',
        'network/network_request_manager_impl.cc',
        'network/network_request_manager_impl.h',
        'network/oauth1_renew_token_request.cc',
        'network/oauth1_renew_token_request.h',
        'network/oauth1_renew_token_response.cc',
        'network/oauth1_renew_token_response.h',
        'network/request_throttler.cc',
        'network/request_throttler.h',
        'network/request_vars_encoder.h',
        'network/request_vars_encoder_impl.cc',
        'network/request_vars_encoder_impl.h',
        'network/response_parser.cc',
        'network/response_parser.h',
        'network/revoke_token_request.cc',
        'network/revoke_token_request.h',
        'network/revoke_token_response.cc',
        'network/revoke_token_response.h',
        'session/session_constants.cc',
        'session/session_constants.h',
        'session/session_state_observer.h',
        'session/persistent_session.h',
        'session/persistent_session_impl.cc',
        'session/persistent_session_impl.h',
        'token_cache/testing_webdata_client.cc',
        'token_cache/testing_webdata_client.h',
        'token_cache/token_cache.h',
        'token_cache/token_cache_factory.cc',
        'token_cache/token_cache_factory.h',
        'token_cache/token_cache_impl.cc',
        'token_cache/token_cache_impl.h',
        'token_cache/token_cache_table.cc',
        'token_cache/token_cache_table.h',
        'token_cache/token_cache_webdata.cc',
        'token_cache/token_cache_webdata.h',
        'token_cache/webdata_client.h',
        'token_cache/webdata_client_factory.cc',
        'token_cache/webdata_client_factory.h',
        'token_cache/webdata_client_impl.cc',
        'token_cache/webdata_client_impl.h',
        'util/constants.cc',
        'util/constants.h',
        'util/init_params.cc',
        'util/init_params.h',
        'util/scope_set.cc',
        'util/scope_set.h',
        'util/token.cc',
        'util/token.h',
        'util/util.cc',
        'util/util.h',
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
        }],
        ['opera_desktop == 1', {
          'dependencies': [
            '<(opera_desktop_dir)/common/sync/sync.gyp:sync_config_impl',
            '<(opera_desktop_dir)/common/opera_constants.gyp:pref_names',
          ],
        }],
      ],
    },
    {
      'target_name': 'oauth2_shared',
      'type': 'static_library',
      'sources': [
        'util/scopes.h',
        'util/scopes.cc',
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
    {
      'target_name': 'oauth2_get_session_name',
      'type': 'static_library',
      'sources': [
        'device_name/get_session_name.cc',
        'device_name/get_session_name.h',
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
      'conditions': [
        ['OS=="mac"', {
          'link_settings': {
            'libraries': [
              # Required by get_session_name_mac.mm on Mac.
              '$(SDKROOT)/System/Library/Frameworks/SystemConfiguration.framework',
            ]
          },
        }],
      ],
    },
    {
      'target_name': 'oauth2_test_support',
      'type': 'static_library',
      'sources': [
        'auth_service_mock.cc',
        'auth_service_mock.h',
        'device_name/device_name_service_mock.cc',
        'device_name/device_name_service_mock.h',
        'network/network_request_manager_mock.cc',
        'network/network_request_manager_mock.h',
        'network/network_request_mock.cc',
        'network/network_request_mock.h',
        'session/persistent_session_mock.cc',
        'session/persistent_session_mock.h',
        'test/testing_constants.cc',
        'test/testing_constants.h',
        'token_cache/token_cache_mock.cc',
        'token_cache/token_cache_mock.h',
      ],
      'dependencies': [
        '<(DEPTH)/testing/gmock.gyp:gmock',
        '<(DEPTH)/testing/gtest.gyp:gtest',
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
          'target_name': 'oauth2_unittests',
          'type': 'executable',
          'sources': [
            'auth_service_impl_test.cc',
            'device_name/device_name_service_impl_test.cc',
            'diagnostics/diagnostic_service_test.cc',
            'migration/oauth1_migrator_impl_test.cc',
            'network/access_token_request_test.cc',
            'network/migration_token_request_test.cc',
            'network/network_request_manager_impl_test.cc',
            'network/oauth1_renew_token_request_test.cc',
            'network/request_throttler_test.cc',
            'network/response_parser_test.cc',
            'network/revoke_token_request_test.cc',
            'network/request_vars_encoder_impl_test.cc',
            'session/persistent_session_impl_test.cc',
            'token_cache/token_cache_impl_test.cc',
            'token_cache/token_cache_table_test.cc',
            'token_cache/token_cache_webdata_test.cc',
            'util/scope_set_test.cc',
            'util/token_test.cc',
          ],
          'dependencies': [
            '<(DEPTH)/base/base.gyp:run_all_unittests',
            '<(DEPTH)/components/components.gyp:keyed_service_core',
            '<(DEPTH)/components/components.gyp:syncable_prefs_test_support',
            '<(DEPTH)/components/prefs/prefs.gyp:prefs_test_support',
            '<(DEPTH)/content/content_shell_and_tests.gyp:test_support_content',
            '<(DEPTH)/net/net.gyp:net_test_support',
            '<(DEPTH)/sql/sql.gyp:test_support_sql',
            '<(opera_common_dir)/sync/sync.gyp:sync_config_test_support',
            '<(opera_common_dir)/sync/sync.gyp:sync_test_support',
            'oauth2',
            'oauth2_test_support',
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
    }],
  ],
}
