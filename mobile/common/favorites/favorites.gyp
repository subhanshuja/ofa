# -*- Mode: python; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
#
# Copyright (C) 2013 Opera Software ASA.  All rights reserved.
#
# This file is an original work developed by Opera Software ASA

{
  'includes': [
    'favorites.gypi',
  ],
  'targets' : [
    {
      'target_name': 'mobile_favorites',
      'type': 'static_library',
      'variables': {
        'chromium_code': 1,
      },
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/sync/sync.gyp:sync',
        '../../../common/bookmarks/bookmarks.gyp:opera_common_bookmarks',
        '../../../common/favorites/favorites.gyp:favorites',
      ],
      'sources': [
        # Defined in favorites.gypi
        '<@(favorites_sources)',
      ],
      'include_dirs': [
        '<(DEPTH)',
        '<(SHARED_INTERMEDIATE_DIR)/protoc_out',
        '<(DEPTH)/third_party/protobuf/src',
        '../../..',
      ],
    }
  ]
}
