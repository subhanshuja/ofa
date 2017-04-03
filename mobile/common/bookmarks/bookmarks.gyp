# -*- Mode: python; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
#
# Copyright (C) 2014 Opera Software ASA.  All rights reserved.
#
# This file is an original work developed by Opera Software ASA

{
  'targets' : [
    {
      'target_name': 'mobile_bookmarks',
      'type': 'static_library',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/components/components.gyp:bookmarks_browser',
        '../../../common/bookmarks/bookmarks.gyp:opera_common_bookmarks',
        '../../../common/suggestion/suggestion.gyp:suggestion',
      ],
      'sources': [
        'bookmark_suggestion_provider.cc',
        'bookmark_suggestion_provider.h',
      ],
      'include_dirs': [
        '<(DEPTH)',
        '../../..',
      ],
    }
  ]
}
