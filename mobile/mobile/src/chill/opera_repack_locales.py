#!/usr/bin/env python

# Since gyp does not support loop, so here this script helps to:
# For each locale, repack it's related pak files, here is:
#  - content_strings_<locale>.pak
#  - ui_strings_<locale>.pak
#  - app_locale_settings_<locale>.pak
# to one single pak file.

import optparse
import os
import sys

chromium_dir = os.path.join(sys.path[0], "../../../../chromium/src")
sys.path.append(os.path.abspath(os.path.join(chromium_dir, "tools/grit")))

from grit.format import data_pack

def calc_inputs(share_int_dir, locale):
  """Determine the files that need processing for the given locale."""
  inputs = []

  #e.g. '<(SHARED_INTERMEDIATE_DIR)/components/strings/
  # components_strings_da.pak',
  inputs.append(os.path.join(share_int_dir, 'components', 'strings',
                  'components_strings_%s.pak' % locale))

  #e.g. '<(SHARED_INTERMEDIATE_DIR)/content/content_strings_da.pak'
  inputs.append(os.path.join(share_int_dir, 'content', 'app', 'strings', 
                  'content_strings_%s.pak' % locale))

  #e.g. '<(SHARED_INTERMEDIATE_DIR)/ui/strings/ui_strings_da.pak',
  inputs.append(os.path.join(share_int_dir, 'ui', 'strings',
                  'ui_strings_%s.pak' % locale))

  #e.g. '<(SHARED_INTERMEDIATE_DIR)/ui/strings/app_locale_settings_da.pak',
  inputs.append(os.path.join(share_int_dir, 'ui', 'strings',
                  'app_locale_settings_%s.pak' % locale))

  #e.g. '<(SHARED_INTERMEDIATE_DIR)/opera_common/product_free_common_strings_da.pak',
  inputs.append(os.path.join(share_int_dir, 'opera_common',
                  'product_free_common_strings_%s.pak' % locale))

  #e.g. '<(SHARED_INTERMEDIATE_DIR)/opera_common/common_strings_da.pak',
  inputs.append(os.path.join(share_int_dir, 'opera_common',
                  'opera_common_strings_%s.pak' % locale))

  return inputs

def calc_output(output_dir, locale):
  return '%s/%s.pak' % (output_dir, locale)

def list_outputs(ouptput_dir, locales):
  outputs = []
  for locale in locales:
    outputs.append(calc_output(ouptput_dir, locale))
  # Quote each element so filename spaces don't mess up gyp's attempt to parse
  # it into a list.
  return " ".join(['"%s"' % x for x in outputs])

def list_inputs(share_int_dir, locales):
  inputs = []
  for locale in locales:
    inputs += calc_inputs(share_int_dir, locale)
  # Quote each element so filename spaces don't mess up gyp's attempt to parse
  # it into a list.
  return " ".join(['"%s"' % x for x in inputs])

def repack_locales(share_int_dir, ouptput_dir, locales):
  """ Loop over and repack the given locales."""
  for locale in locales:
    inputs = []
    inputs += calc_inputs(share_int_dir, locale)
    output = calc_output(ouptput_dir, locale)
    data_pack.DataPack.RePack(output, inputs)

def DoMain(argv):
  parser = optparse.OptionParser("usage: %prog [options] locales")

  parser.add_option("-i", action="store_true", dest="inputs", default=False,
                    help="Print the expected input file list, then exit.")

  parser.add_option("-o", action="store_true", dest="outputs", default=False,
                    help="Print the expected output file list, then exit.")

  parser.add_option("-s", action="store", dest="share_int_dir",
                    help="Shared intermediate build files output directory.")

  parser.add_option("-d", action="store", dest="ouptput_dir",
                    help="The output directory.")

  options, locales = parser.parse_args(argv)

  locales = " ".join(locales).split()

  if not locales:
    parser.error('Please specificy at least one locale to process.\n')

  print_inputs = options.inputs
  print_outputs = options.outputs
  ouptput_dir = options.ouptput_dir
  share_int_dir = options.share_int_dir

  if not (ouptput_dir and share_int_dir ):
    parser.error('Please specify all of "-s" and "-d".\n')

  if print_inputs and print_outputs:
    parser.error('Please specify only one of "-i" or "-o".\n')

  if print_inputs:
    return list_inputs(share_int_dir, locales)

  if print_outputs:
    return list_outputs(ouptput_dir, locales)

  return repack_locales(share_int_dir, ouptput_dir, locales)

if __name__ == '__main__':
  results = DoMain(sys.argv[1:])
  if results:
    print results
