# Copyright (C) 2015 Opera Software ASA.  All rights reserved.
#
# This file is an original work developed by Opera Software ASA

{
  'targets': [
    {
      'target_name': 'message_generator',
      'type': 'static_library',
      'hard_dependency': 1,
      'include_dirs': [
        '../..',
        '<(DEPTH)',
      ],
      'dependencies': [
        '<(DEPTH)/content/content.gyp:content_common',
        '<(DEPTH)/ui/accessibility/accessibility.gyp:ax_gen',
      ],
      'sources': [
        'all_messages.h',
        'message_generator.cc',
      ],
    },
  ],
}
