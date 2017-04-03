# -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
#
# Copyright (C) 2014 Opera Software ASA.  All rights reserved.
#
# This file is an original work developed by Opera Software ASA

{
  'variables': {
    'opera_root_dir': '../..',
  },
  'targets' : [
    {
      'target_name': 'opera_common_bookmarks',
      'type': 'static_library',
      'dependencies': [
        '<(DEPTH)/components/components.gyp:bookmarks_browser',
        '<(DEPTH)/sync/sync.gyp:sync_proto',
        '../sync/sync.gyp:sync_status',
      ],
      'sources': [
        'calculate_hash_task.cc',
        'calculate_hash_task.h',
        'duplicate_sync_helper.cc',
        'duplicate_sync_helper.h',
        'duplicate_task.cc',
        'duplicate_task.h',
        'duplicate_task_runner.cc',
        'duplicate_task_runner.h',
        'duplicate_task_runner_observer.h',
        'duplicate_tracker.cc',
        'duplicate_tracker.h',
        'duplicate_tracker_delegate.h',
        'duplicate_tracker_factory.cc',
        'duplicate_tracker_factory.h',
        'duplicate_util.cc',
        'duplicate_util.h',
        'original_elector.cc',
        'original_elector.h',
        'original_elector_local.cc',
        'original_elector_local.h',
        'original_elector_sync.cc',
        'original_elector_sync.h',
        'remove_duplicate_task.cc',
        'remove_duplicate_task.h',
        'tracker_observer.h',
      ],
      'include_dirs': [
        '<(DEPTH)',
        '../..',
      ],
      'conditions': [
        ['opera_desktop == 1', {
          'dependencies': [
            '<(opera_root_dir)/desktop/opera_resources.gyp:opera_strings_gen',
          ],
        }],
      ],
    },
  ]
}
