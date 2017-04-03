# -*- Mode: python; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
#
# Copyright (C) 2015 Opera Software ASA.  All rights reserved.
#
# This file is an original work developed by Opera Software ASA

# This filed is shared between gyp and gn and must not contain conditionals
# or other complex gyp constructs.
{
  'variables': {
    'favorites_sources': [
      'collection_reader.cc',
      'collection_reader.h',
      'device_folder_impl.cc',
      'device_folder_impl.h',
      'device_root_folder_impl.cc',
      'device_root_folder_impl.h',
      'favorite.cc',
      'favorite.h',
      'favorite_duplicate_tracker_delegate.cc',
      'favorite_duplicate_tracker_delegate.h',
      'favorite_suggestion_provider.cc',
      'favorite_suggestion_provider.h',
      'favorites.cc',
      'favorites.h',
      'favorites_delegate.h',
      'favorites_export.h',
      'favorites_impl.cc',
      'favorites_impl.h',
      'favorites_observer.h',
      'folder.cc',
      'folder.h',
      'folder_impl.cc',
      'folder_impl.h',
      'meta_info.cc',
      'meta_info.h',
      'node_favorite_impl.cc',
      'node_favorite_impl.h',
      'node_folder_impl.cc',
      'node_folder_impl.h',
      'root_folder_impl.cc',
      'root_folder_impl.h',
      'savedpage.cc',
      'savedpage.h',
      'savedpage_impl.cc',
      'savedpage_impl.h',
      'savedpages.cc',
      'savedpages.h',
      'special_folder_impl.cc',
      'special_folder_impl.h',
      'title_match.cc',
      'title_match.h',
    ],
  }
}
