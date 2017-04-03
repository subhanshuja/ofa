# -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
#
# Copyright (C) 2013 Opera Software ASA.  All rights reserved.
#
# This file is an original work developed by Opera Software ASA

{
  'targets' : [
    {
      'target_name': 'favorites',
      'type': 'static_library',
      'dependencies': [
        '<(DEPTH)/sql/sql.gyp:sql',
        '<(DEPTH)/url/url.gyp:url_lib',
        '../suggestion/suggestion.gyp:suggestion',
      ],
      'sources': [
        'favorite.cc',
        'favorite_collection.h',
        'favorite_collection_impl.cc',
        'favorite_collection_impl.h',
        'favorite_collection_observer.h',
        'favorite_data.cc',
        'favorite_data.h',
        'favorite_db_storage.cc',
        'favorite_db_storage.h',
        'favorite.h',
        'favorite_index.cc',
        'favorite_index.h',
        'favorite_storage.cc',
        'favorite_storage.h',
        'favorite_suggestion_provider.cc',
        'favorite_suggestion_provider.h',
        'favorite_utils.cc',
        'favorite_utils.h',
      ],
      'include_dirs': [
        '<(DEPTH)',
        '../..',
      ],
    },
    {
      'target_name': 'favorites_unittests',
      'type': 'executable',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:run_all_unittests',
        '<(DEPTH)/testing/gmock.gyp:gmock',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        'favorites',
      ],
      'include_dirs': [
        '<(DEPTH)',
      ],
      'sources': [
      ],
    },
  ]
}
