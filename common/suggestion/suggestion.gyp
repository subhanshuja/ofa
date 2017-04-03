# -*- Mode: python; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
#
# Copyright (C) 2013 Opera Software ASA.  All rights reserved.
#
# This file is an original work developed by Opera Software ASA

{
  'targets' : [
    {
      'target_name': 'suggestion',
      'type': 'static_library',
      'variables': {
        'chromium_code': 1,
        'use_icu_alternatives%': 0,
      },
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
      ],
      'sources': [
        'query_parser.cc',
        'query_parser.h',
        'snippet.h',
        'suggestion_callback.h',
        'suggestion_item.cc',
        'suggestion_item.h',
        'suggestion_manager.cc',
        'suggestion_manager.h',
        'suggestion_provider.h',
        'suggestion_tokens.cc',
        'suggestion_tokens.h',
        'url_index.cc',
        'url_index.h',
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
        ['OS == "ios"', {
          'sources': [
            '<(DEPTH)/net/base/escape.cc',
            '<(DEPTH)/net/base/escape.h',
          ],
        }],
        ['use_icu_alternatives==0', {
          'dependencies+': [
            '<(DEPTH)/base/base.gyp:base_i18n',
          ],
        }],
      ],
    },
    {
      'target_name': 'suggestion_unittests',
      'type': 'executable',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:run_all_unittests',
        '<(DEPTH)/testing/gmock.gyp:gmock',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        'suggestion',
      ],
      'include_dirs': [
        '<(DEPTH)',
      ],
      'sources': [
        'suggestion_manager_test.cc',
      ],
    },
  ]
}
