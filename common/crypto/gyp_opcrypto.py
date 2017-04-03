#!/usr/bin/env python
#
# Copyright (C) 2012 Opera Software ASA.  All rights reserved.
#
# This file is an original work developed by Opera Software ASA

"""Generates the project, the easy way."""

import os
import subprocess
import sys

script_dir = os.path.dirname(os.path.realpath(__file__))
gyp_path = os.path.join(script_dir, 'opcrypto.gyp')


def get_repo_root(path):
  """Get the absolute path to the repository of the current script."""
  root_dir = os.path.dirname(path)
  while (root_dir != os.path.dirname(root_dir) and
         not os.path.exists(os.path.join(root_dir, ".git"))):
    root_dir = os.path.dirname(root_dir)
  return root_dir

if __name__ == '__main__':
  # Absolute path of the chromium/src directory.
  chromium_src = os.path.join(get_repo_root(script_dir), 'chromium', 'src')

  # Absolute path to the chromium/src/build/gyp_chromium script.
  chromium_script = os.path.join(chromium_src, 'build', 'gyp_chromium')

  # Construct arguments list.
  args = [sys.executable,  # On Windows we can't just run the script itself.
          chromium_script, gyp_path, '--depth=%s' % (chromium_src)]

  # Add platform specific includes.
  if sys.platform == 'win32' or sys.platform == 'cygwin':
    args.extend(['-I',
                 os.path.join(script_dir, 'windows', 'opera_global_win.gypi')])

  # Add our custom python modules.
  python_paths = [ os.path.join(script_dir, 'build') ]
  if 'PYTHONPATH' in os.environ:
      python_paths.append(os.environ['PYTHONPATH'])
  os.environ['PYTHONPATH'] = os.path.pathsep.join(python_paths)

  # Run the script.
  print args[1] + " " + args[2] + " " + args[3];
  p = subprocess.Popen(args, shell=False, cwd=chromium_src)
  p.communicate()
  sys.exit(p.returncode)
