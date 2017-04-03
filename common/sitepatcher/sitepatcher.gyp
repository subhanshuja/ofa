# Copyright (C) 2015 Opera Software ASA.  All rights reserved.
#
# This file is an original work developed by Opera Software ASA

{
  'targets': [
    {
      'target_name': 'browser',
      'type': 'static_library',
      'include_dirs': [
        '../..',
        '<(DEPTH)',
      ],
      'dependencies': [
        '../crypto/opcrypto.gyp:opcrypto',
        '../message_generator/message_generator.gyp:message_generator',
      ],
      'sources': [
        'browser/browserjs_key.h',
        'browser/op_site_patcher.cc',
        'browser/op_site_patcher_config.cc',
        'browser/op_site_patcher_config.h',
        'browser/op_site_patcher.h',
        'browser/op_update_checker.cc',
        'browser/op_update_checker_client.cc',
        'browser/op_update_checker.h',
        'browser/op_update_checker_register_prefs.cc',
        'browser/op_update_checker_register_prefs.h',
        'browser/op_update_downloader.cc',
        'browser/op_update_downloader.h',
        'browser/site_patch_component_installer.cc',
        'browser/site_patch_component_installer.h',
        'config.h',
      ],
      'conditions': [
        ['OS == "android"', {
          'sources!': [
            'browser/op_update_checker_client.cc',
            'browser/op_update_checker_register_prefs.cc',
            'browser/op_update_checker_register_prefs.h',
            'browser/site_patch_component_installer.cc',
            'browser/site_patch_component_installer.h',
          ],
        }, {
          'sources!': [
            'browser/op_update_checker.cc',
            'browser/op_update_checker.h',
            'browser/op_update_downloader.cc',
            'browser/op_update_downloader.h',
          ],
        }],
      ],
    },
    {
      'target_name': 'renderer',
      'type': 'static_library',
      'include_dirs': [
        '../..',
        '<(DEPTH)',
      ],
      'dependencies': [
        '<(DEPTH)/third_party/WebKit/public/blink.gyp:blink',
        '<(DEPTH)/ui/accessibility/accessibility.gyp:ax_gen',
      ],
      'sources': [
        'renderer/op_script_injector.cc',
        'renderer/op_script_injector.h',
        'renderer/op_script_store.cc',
        'renderer/op_script_store.h',
        'renderer/op_site_prefs.cc',
        'renderer/op_site_prefs.h',
      ],
      'defines': [
        'OPR_VERSION="<(opr_version)"',
      ],
    },
  ],
}
