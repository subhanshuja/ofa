#!/usr/bin/env python
#
# Copyright (C) 2014 Opera Software ASA.  All rights reserved.
#
# This file is an original work developed by Opera Software ASA

from __future__ import print_function
import hashlib
import os
import re
import subprocess
import sys

# Make SRC_DIR point to the chromium's "src" directory.
SRC_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
chromium = os.path.dirname(SRC_DIR)

jars = [ 'accessibility_test_framework',
         'apache_velocity',
         'bouncycastle',
         'byte_buddy',
         'guava',
         'hamcrest',
         'icu4j',
         'intellij',
         'objenesis',
         'ow2_asm',
         'robolectric_libs',
         'sqlite4java' ]

# Trick DEPS.
def Var(_str):
  return '%s'

def is_executable_file(path):
  return os.path.isfile(path) and os.access(path, os.X_OK)

# Find executable in PATH environment variable.
def find_executable(name):
  for search_path in os.environ["PATH"].split(os.pathsep):
    path = os.path.join(search_path, name)
    if is_executable_file(path):
      return path

def RunOperaHooks(args):
  is_opera_build = False
  for arg in args:
    if arg.endswith('opera.gyp'):
      is_opera_build = True

  DEPS = os.path.join(SRC_DIR, 'DEPS')
  execfile(DEPS)  # this ends up defining 'hooks' in locals().

  platform = sys.platform
  if sys.platform == 'cygwin':
    platform = 'win32'
  # Only use shell when it's win32 command line and not cygwin.
  shell = sys.platform == "win32"

  for hook in locals()['hooks']: # pylint: disable-msg=E0602
    name = hook.get('name', '')
    if name == 'clang' and 'SKIP_CLANG_DOWNLOAD' not in os.environ:
      try:
        out = subprocess.check_output(hook['action'], cwd=chromium,
                                      shell=shell, stderr=subprocess.STDOUT)
        print(out)
      except subprocess.CalledProcessError as e:
        print(e.output)
        raise

    elif name == 'android_support_test_runner':
      subprocess.check_call(hook['action'], cwd=chromium, shell=shell)

    elif name == 'remove_stale_pyc_files':
      # extra directories to purge
      for d in ['src/build']:
        if d not in hook['action']:
          hook['action'].append(d)
      if sys.platform.startswith('linux'):
        # the upstream hook uses os.walk which is slow. We tend to run
        # the hooks more often, so let's save the half-second here by
        # using find instead which is fairly straightforward on linux.
        dirs = hook['action'][1:]
        find_invocation = [ 'find' ] + dirs + [ '-name', '*.pyc' ]
        pycs = subprocess.check_output(find_invocation, cwd=chromium)
        for pyc in pycs.splitlines():
          pyc = os.path.join(chromium, pyc)
          py = pyc[:-3] + 'py'
          if not os.path.exists(py):
            try:
              os.remove(pyc)
            except OSError:
              pass
        # upstream hook also removes empty dirs, so do that too
        delete_dirs = [ 'find' ] + dirs + [ '-type', 'd', '-empty', '-delete' ]
        subprocess.check_call(delete_dirs, cwd=chromium)
      else:
        subprocess.check_call(hook['action'], cwd=chromium, shell=shell)

    elif (name in jars or
          name == 'binutils' and sys.platform.startswith('linux')) or \
         (name == 'win_toolchain' and sys.platform == "win32") or \
         (not is_opera_build and name == 'syzygy-binaries' and \
              sys.platform == "win32"):
      subprocess.check_call(hook['action'], cwd=chromium, shell=shell)

    elif (name.startswith('gn')) or \
         (name.startswith('clang_format') and \
          'SKIP_CLANG_DOWNLOAD' not in os.environ):
      # Only run the hooks with matched platforms, although these hooks have
      # their on platform checking we should run as few code as possible.
      match_platform = False
      argv = hook['action']
      for arg in argv:
        if arg.startswith('--platform='):
          if re.match(arg[len('--platform='):], platform):
            match_platform = True
          break
      if not match_platform:
        continue

      sha1_file = os.path.join(chromium, argv[-1])
      should_download = True
      executable = os.path.join(chromium, os.path.splitext(sha1_file)[0])
      if os.path.isfile(executable):
        executable_str = open(executable, 'rb').read()
        sha1_str = open(os.path.join(chromium, sha1_file), 'r').readline()
        digest = hashlib.sha1(executable_str).hexdigest()
        # For now the hook will always try to download which is quite slow
        # and always require network connection, don't do that if the file
        # already exist and has exactly the same SHA1 as required.
        if digest == sha1_str.strip():
          should_download = False

      if should_download:
        # For some reason, download_from_google_storage.bat in depot_tools
        # doesn't work for us as it assumed a 'python' executable under
        # depot_tools directory, which isn't the case for us. We have to
        # workaround this on Windows by replacing argv[0] with python call
        # directly.
        if sys.platform == "win32":
          for dir in os.environ['PATH'].split(os.path.pathsep):
            p = os.path.join(dir, '%s.py' % argv[0])
            if os.path.isfile(p):
              argv[0] = p
              break
          argv = [ sys.executable ] + argv
        elif not is_executable_file(argv[0]) and not find_executable(argv[0]):
          print("ERROR: missing tool required for build: %s" % argv[0])

          if argv[0] == 'download_from_google_storage':
            print("""
Note: %s is provided by Google depot_tools which
      A) should be available via the PATH environment variable, and
      B) should be updated to the latest version.

      For installation instructions, see:
      http://www.chromium.org/developers/how-tos/install-depot-tools

""" % argv[0])

          sys.exit(1)

        subprocess.check_call(argv, cwd=chromium, shell=shell)

if __name__ == '__main__':
  RunOperaHooks(sys.argv)
