#!/usr/bin/env python

""" Creates a zip file in the staging dir with the result of a compile.
    It can be sent to other machines for testing.
"""

import csv
import glob
import optparse
import os
import re
import shutil
import stat
import sys

import opera_utils

PATH_FILTERS = {
    'dummy': None,
}

def CopyDebugCRT(build_dir):
  # Copy the relevant CRT DLLs to |build_dir|. We copy DLLs from all versions
  # of VS installed to make sure we have the correct CRT version, unused DLLs
  # should not conflict with the others anyways.
  crt_dlls = glob.glob(
      'C:\\Program Files (x86)\\Microsoft Visual Studio *\\VC\\redist\\'
      'Debug_NonRedist\\x86\\Microsoft.*.DebugCRT\\*.dll')
  for dll in crt_dlls:
    shutil.copy(dll, build_dir)

def FileRegexWhitelist(options):
  if opera_utils.IsWindows() and options.target is 'Release':
    # Special case for chrome. Add back all the chrome*.pdb files to the list.
    # Also add browser_test*.pdb, ui_tests.pdb and ui_tests.pdb.
    # TODO(nsylvain): This should really be defined somewhere else.
    return (r'^(chrome[_.]dll|chrome[_.]exe'
            # r'|browser_test.+|unit_tests'
            # r'|chrome_frame_.*tests'
            r')\.pdb$')

  return '$NO_FILTER^'

def FileRegexBlacklist(options):
  if opera_utils.IsWindows():
    # Remove all .ilk/.7z and maybe PDB files
    # TODO(phajdan.jr): Remove package_pdb_files when nobody uses it.
    include_pdbs = False; #options.factory_properties.get('package_pdb_files', True)
    if include_pdbs:
      return r'^.+\.(ilk|7z|(precompile\.h\.pch.*))$'
    else:
      return r'^.+\.(ilk|pdb|7z|(precompile\.h\.pch.*))$'
  if opera_utils.IsMac():
    # The static libs are just built as intermediate targets, and we don't
    # need to pull the dSYMs over to the testers most of the time (except for
    # the memory tools).
    include_dsyms = False #options.factory_properties.get('package_dsym_files', False)
    if include_dsyms:
      return r'^.+\.(a)$'
    else:
      return r'^.+\.(a|dSYM)$'
  if opera_utils.IsLinux():
    # object files, archives, and gcc (make build) dependency info.
    return r'^.+\.(o|a|d)$'

  return '$NO_FILTER^'


def FileExclusions():
  all_platforms = ['.landmines', 'obj', 'gen']
  # Skip files that the testers don't care about. Mostly directories.
  if opera_utils.IsWindows():
    # Remove obj or lib dir entries
    return all_platforms + ['cfinstaller_archive', 'lib', 'installer_archive']
  if opera_utils.IsMac():
    return all_platforms + [
      # We don't need the arm bits v8 builds.
      'd8_arm', 'v8_shell_arm',
      # pdfsqueeze is a build helper, no need to copy it to testers.
      'pdfsqueeze',
      # We copy the framework into the app bundle, we don't need the second
      # copy outside the app.
      # TODO(mark): Since r28431, the copy in the build directory is actually
      # used by tests.  Putting two copies in the .zip isn't great, so maybe
      # we can find another workaround.
      # 'Chromium Framework.framework',
      # 'Google Chrome Framework.framework',
      # We copy the Helper into the app bundle, we don't need the second
      # copy outside the app.
      'Chromium Helper.app',
      'Google Chrome Helper.app',
      '.deps', 'obj.host', 'obj.target', 'lib'
    ]
  if opera_utils.IsLinux():
    return all_platforms + [
      # intermediate build directories (full of .o, .d, etc.).
      'appcache', 'glue', 'googleurl', 'lib.host', 'obj.host',
      'obj.target', 'src', '.deps',
      # scons build cruft
      '.sconsign.dblite',
      # build helper, not needed on testers
      'mksnapshot',
    ]

  return all_platforms

def MakeArchive(build_dir, staging_dir, zip_file_list,
                           zip_file_name, path_filter):
  """Creates a full build archive.
  Returns the path of the created archive."""
  (zip_dir, zip_file) = opera_utils.MakeZip(staging_dir,
                                               zip_file_name,
                                               zip_file_list,
                                               build_dir,
                                               raise_error=True,
                                               path_filter=path_filter)
  opera_utils.RemoveDirectory(zip_dir)
  if not os.path.exists(zip_file):
    raise StagingError('Failed to make zip package %s' % zip_file)
  opera_utils.MakeWorldReadable(zip_file)

  # Report the size of the zip file to help catch when it gets too big and
  # can cause bot failures from timeouts during downloads to testers.
  zip_size = os.stat(zip_file)[stat.ST_SIZE]
  print 'Zip file is %ld bytes' % zip_size

  return zip_file

class PathMatcher(object):
  """Generates a matcher which can be used to filter file paths."""

  def __init__(self, options):
    def CommaStrParser(val):
      return [f.strip() for f in csv.reader([val]).next()]
    self.inclusions = CommaStrParser(options.include_files)
    self.exclusions = CommaStrParser(options.exclude_files) + FileExclusions()

    self.regex_whitelist = FileRegexWhitelist(options)
    self.regex_blacklist = FileRegexBlacklist(options)
    self.exclude_unmatched = options.exclude_unmatched

  def __str__(self):
    return '\n  '.join([
        'Zip rules',
        'Inclusions: %s' % self.inclusions,
        'Exclusions: %s' % self.exclusions,
        "Whitelist regex: '%s'" % self.regex_whitelist,
        "Blacklist regex: '%s'" % self.regex_blacklist,
        'Zip unmatched files: %s' % (not self.exclude_unmatched)])

  def Match(self, filename):
    if filename in self.inclusions:
      return True
    if filename in self.exclusions:
      return False
    if re.match(self.regex_whitelist, filename):
      return True
    if re.match(self.regex_blacklist, filename):
      return False
    return not self.exclude_unmatched


def GetStagingDir(start_dir):
  """Creates a chrome_staging dir in the starting directory. and returns its
  full path.
  """
  staging_dir = os.path.join(start_dir, 'chrome_staging')
  opera_utils.MaybeMakeDirectory(staging_dir)
  return staging_dir

def CopyDebugCRT(build_dir):
  # Copy the relevant CRT DLLs to |build_dir|. We copy DLLs from all versions
  # of VS installed to make sure we have the correct CRT version, unused DLLs
  # should not conflict with the others anyways.
  crt_dlls = glob.glob(
      'C:\\Program Files (x86)\\Microsoft Visual Studio *\\VC\\redist\\'
      'Debug_NonRedist\\x86\\Microsoft.*.DebugCRT\\*.dll')
  for dll in crt_dlls:
    shutil.copy(dll, build_dir)

def Archive(options):
  src_dir = os.path.abspath(options.src_dir)

  build_dir = options.build_dir
  build_dir = os.path.abspath(os.path.join(build_dir, options.target))

  staging_dir = GetStagingDir(src_dir)

  unversioned_base_name = 'chromium'
  zip_file_name = unversioned_base_name + '-' + options.build_version + '-' + options.build_datetime + '-' + options.target.lower()

  print 'Full Staging in %s' % staging_dir
  print 'Build Directory %s' % build_dir

  # Copy the crt files if necessary.
  if options.target == 'Debug' and opera_utils.IsWindows():
    CopyDebugCRT(build_dir)

  # Build the list of files to archive.
  root_files = os.listdir(build_dir)
  path_filter = PathMatcher(options)
  print path_filter
  print ('\nActually excluded: %s' %
         [f for f in root_files if not path_filter.Match(f)])

  zip_file_list = [f for f in root_files if path_filter.Match(f)]
  zip_file = MakeArchive(build_dir, staging_dir, zip_file_list,
                                    zip_file_name, options.path_filter)

  return 0


def main(argv):
  option_parser = optparse.OptionParser()
  option_parser.add_option('--target',
                           help='build target to archive (Debug or Release)')
  option_parser.add_option('--src-dir', default='.',
                           help='path to the top-level sources directory')
  option_parser.add_option('--build-dir', default=os.path.join('build', 'out'),
                           help=('path to main build directory (the parent of '
                                 'the Release or Debug directory)'))
  option_parser.add_option('--exclude-files', default='',
                           help=('Comma separated list of files that should '
                                 'always be excluded from the zip.'))
  option_parser.add_option('--include-files', default='',
                           help=('Comma separated list of files that should '
                                 'always be included in the zip.'))
  option_parser.add_option('--webkit-dir',
                           help='webkit directory path, relative to --src-dir')
  option_parser.add_option('--path-filter',
                           help='Filter to use to transform build zip '
                                '(avail: %r).' % list(PATH_FILTERS.keys()))
  option_parser.add_option('--exclude-unmatched', action='store_true',
                           help='Exclude all files not matched by a whitelist')
  option_parser.add_option('--build-version', default='',
                           help='Chromium version number')
  option_parser.add_option('--build-datetime', default='',
                           help='Time and date of build')

  options, args = option_parser.parse_args(argv)

  if not options.target:
    options.target = options.factory_properties.get('target', 'Release')

  # When option_parser is passed argv as a list, it can return the caller as
  # first unknown arg.  So throw a warning if we have two or more unknown
  # arguments.
  if args[1:]:
    print 'Warning -- unknown arguments' % args[1:]

  return Archive(options)


if '__main__' == __name__:
  sys.exit(main(sys.argv))
