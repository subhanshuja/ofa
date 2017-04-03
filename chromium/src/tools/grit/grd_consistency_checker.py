#!/usr/bin/env python
# Copyright (C) 2015 Opera Software ASA.  All rights reserved.
#
# This file is an original work developed by Opera Software ASA

"""Checks if all *.grd files given on input contain the same resources in the
   same order. Prints differences on standard output."""

import argparse
import os
import os.path
import sys
import xml.etree.ElementTree as ElementTree

_STATUS_CONSISTENT = 0
_STATUS_ERROR = 1
_STATUS_DIFFS_FOUND = 2

class GrdFile:
  def __init__(self, name):
    if not os.path.isfile(name):
      raise Exception('file [%s] not found' % name)
    self.name = name

    try:
      root = ElementTree.parse(name).getroot()
    except Exception, e:
      raise Exception('error while parsing [%s]: %s' % (name, e))

    # Retrieve all resource names from given file. Note: GRD files are read
    # without interpreting them in any way (in particular no conditions are
    # checked).
    INTERESTING_TAGS = ('message', 'structure', 'include')
    try:
      self.resource_names = [ tag.attrib['name'] for tag in root.iter() if tag.tag in INTERESTING_TAGS ]
    except KeyError:
      raise Exception('[%s] contains unnamed resource' % name)

  def HasSameResources(self, grd_file):
    """Checks if |grd_file| defines the same resources. Prints the list of found
       differences on standard output.
       Returns False when files don't define the same set of resources."""
    files_have_same_resources = True
    for file1, file2 in ((self, grd_file), (grd_file, self)):
      diff = set(file1.resource_names).difference(set(file2.resource_names))
      if diff:
        print 'Resources present in [%s] which are missing from [%s]:' % (file1.name, file2.name)
        for resource_name in diff:
          print '\t"%s"' % resource_name
        print
        files_have_same_resources = False

    return files_have_same_resources

  def HasSameResourceOrder(self, grd_file):
    """Checks if order of the resources in both files is the same. Prints first
       found difference on standard output.
       Returns False if order of resources in both files is different."""
    for base_resource, resource in zip(self.resource_names, grd_file.resource_names):
      if base_resource != resource:
        print 'Wrong resource order, found "%s" in [%s] and "%s" in [%s], ' % (base_resource, self.name, resource, grd_file.name)
        return False
    return True

def CheckConsistency(grd_files):
  return_value = _STATUS_CONSISTENT
  assert(len(grd_files) > 1)

  base_grd_file = grd_files[0]
  for grd_file in grd_files[1:]:
    # Check if both files contain the same resources.
    resources_consistent = base_grd_file.HasSameResources(grd_file)
    if not resources_consistent:
      return_value = _STATUS_DIFFS_FOUND
      continue

    if not base_grd_file.HasSameResourceOrder(grd_file):
      return_value = _STATUS_DIFFS_FOUND
      continue

  return return_value

def DoMain(argv):
  parser = argparse.ArgumentParser(description='Checks if passed grd files contain the same resources.')
  parser.add_argument('FILE', nargs='+')
  parser.add_argument('--stamp-file', help='path to an empty file that shall be created if all files are consistent')
  args = parser.parse_args(argv)

  if len(args.FILE) == 1:
    print 'error, too few files given'
    return _STATUS_ERROR

  if len(args.FILE) != len(set(args.FILE)):
    print 'Warning: input file list contains duplicates (which shall be removed).'
    args.FILE = list(set(args.FILE))

  result = CheckConsistency(map(GrdFile, args.FILE))
  if result == _STATUS_CONSISTENT:
    if args.stamp_file:
      open(args.stamp_file, 'w')
    else:
      print 'All files are consistent.'
  return result

if __name__ == '__main__':
  try:
    sys.exit(DoMain(sys.argv[1:]))
  except Exception, e:
    print e
