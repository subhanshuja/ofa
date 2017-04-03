# -*- Mode: python; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
#
# Copyright (C) 2014 Opera Software ASA.  All rights reserved.
#
# This file is an original work developed by Opera Software ASA

{
  'targets' : [
    {
      'target_name': 'url_color_util',
      'type': 'static_library',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/url/url.gyp:url_lib',
      ],
      'include_dirs': [
        '<(DEPTH)',
      ],
      'sources': [
        'url_hasher.cc',
        'url_hasher.h',
        'url_color_table.cc',
        'url_color_table.h',
        'url_fall_back_colors.cc',
        'url_fall_back_colors.h',
      ],
    },
    {
      'target_name': 'url_color_util_unittest',
      'type': 'executable',
      'dependencies': [
        'url_color_util',
        '<(DEPTH)/base/base.gyp:run_all_unittests',
        '<(DEPTH)/testing/gtest.gyp:gtest',
      ],
      'include_dirs': [
        '<(DEPTH)',
      ],
      'sources': [
        'url_color_table_unittest.cc',
        'url_fall_back_colors_unittest.cc',
        'url_hasher_unittest.cc',
      ],
    },
  ]
}
