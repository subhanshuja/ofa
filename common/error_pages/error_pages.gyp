# Copyright (C) 2015 Opera Software ASA.  All rights reserved.
#
# This file is an original work developed by Opera Software ASA

{
  'variables': {
    'enable_search_in_error_page%': 0,
    },
  'targets': [
    {
      'target_name': 'error_pages',
      'type': 'static_library',
      'include_dirs': [
        '../..',
        '<(DEPTH)',
      ],
      'dependencies': [
        '<(DEPTH)/skia/skia.gyp:skia',
        '../resources/resources.gyp:resources',
        '../strings/strings.gyp:strings',
        '<(DEPTH)/base/base.gyp:base_i18n',
        '<(DEPTH)/components/components.gyp:ssl_errors',
      ],
      'sources': [
        'ssl/ssl_blocking_page.cc',
        'ssl/ssl_blocking_page.h',
        'error_page.cc',
        'error_page.h',
        'error_page_search_data_provider.h',
        'localized_error.cc',
        'localized_error.h',
      ],
      'conditions': [
        ['enable_search_in_error_page==1', {
          'defines': ['ENABLE_SEARCH_IN_ERROR_PAGE', ],
        }],
      ],
    },
  ],
}
