#!/usr/bin/env python

# Copyright (C) 2016 Opera Software ASA.  All rights reserved.
# This file is an original work developed by Opera Software ASA.

"""Prepare GN args for Chromium product and run 'gn gen'."""

import os
import platform
import subprocess
import sys

script_dir = os.path.dirname(os.path.realpath(__file__))

def get_gn_cmd(chromium_src):
  platform_name = platform.system().lower()
  if platform_name == 'windows':
    return os.path.join(chromium_src, 'buildtools', 'win', 'gn.exe')
  elif platform_name == 'linux':
    return os.path.join(chromium_src, 'buildtools', 'linux64', 'gn')
  elif platform_name == 'darwin':
    return os.path.join(chromium_src, 'buildtools', 'mac', 'gn')
  else:
    print('Unknown platform: %s' % platform_name)
    exit(1)


if __name__ == '__main__':
  if len(sys.argv) < 2:
    print('Usage: %s out_dir [extra_gn_args]' % sys.argv[0])
    print('Example: %s out/Debug is_debug=true' % sys.argv[0])
    exit(1)

  sys.path.insert(0, os.path.join(script_dir, '..'))
  from run_opera_hooks import RunOperaHooks
  RunOperaHooks(sys.argv[1:])

  chromium_src = os.path.normpath(os.path.join(script_dir, '..', '..'))
  gn_cmd = get_gn_cmd(chromium_src)

  out_dir = os.path.normpath(os.path.join(chromium_src, sys.argv[1]))
  if not os.path.exists(out_dir):
    os.makedirs(out_dir)

  if len(sys.argv) > 2:
    with open(os.path.join(out_dir, 'args.gn'), 'w') as args_file:
      for arg in sys.argv[2:]:
        args_file.write('%s\n' % arg)

  print("gn_cmd=%s" % gn_cmd)

  p = subprocess.Popen([gn_cmd, 'gen', sys.argv[1]], shell=False,
                       cwd=chromium_src)
  p.communicate()
  sys.exit(p.returncode)
