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
      'target_name': 'resources',
      'type': 'none',
      'actions': [
        # Data resources.
        {
          'action_name': 'common_resources',
          'variables': {
            'grit_grd_file': 'opera_common_resources.grd',
          },
          'includes': [ '../../chromium/src/build/grit_action.gypi' ],
        },
      ],
    },
  ],
}
