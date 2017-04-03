# Copyright (C) 2015 Opera Software ASA.  All rights reserved.
#
# This file is an original work developed by Opera Software ASA

{
  'variables': {
    'grit_out_dir': '<(SHARED_INTERMEDIATE_DIR)/opera_common',
    'grit_resource_ids%': 'resource_ids',
  },
  'targets': [
    {
      # Note: generated pak file should be repacked by your project file.
      'target_name': 'strings',
      'type': 'none',
      'actions': [
        # Localizable resources.
        {
          'action_name': 'check_common_product_related_strings_consistency',
          'variables': {
            'stamp_file': '<(grit_out_dir)/product_related_strings_consistent.stamp',
            'grd_files' : [
              'opera_common_strings.grd',
              'safezone_common_strings.grd',
            ],
          },
          'includes' : [
            '../../chromium/src/build/grd_consistency_checker_action.gypi',
          ],
        },
        {
          'action_name': 'product_free_common_strings',
          'variables': {
            'grit_grd_file': 'product_free_common_strings.grd',
          },
          'includes': [ '../../chromium/src/build/grit_action.gypi' ],
        },
        {
          'action_name': 'opera_common_strings',
          'variables': {
            'grit_grd_file': 'opera_common_strings.grd',
          },
          'includes': [ '../../chromium/src/build/grit_action.gypi' ],
        },
      ],
    },
    {
      'target_name': 'strings_map',
      'type': 'static_library',
      'dependencies': ['strings'],
      'include_dirs': [
        '<(DEPTH)',
        '<(grit_out_dir)'
      ],
      'sources': [
        '<(grit_out_dir)/grit/product_free_common_strings_resources_map.cc',
        '<(grit_out_dir)/grit/product_related_common_strings_resources_map.cc',
        ],
    },
  ],
}
