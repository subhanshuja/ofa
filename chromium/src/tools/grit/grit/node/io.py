#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''The <output> and <file> elements.
'''

import os
import sys

import grit.format.rc_header

from grit import xtb_reader
from grit.node import base


class FileNode(base.Node):
  '''A <file> element.'''

  def __init__(self):
    super(FileNode, self).__init__()
    self.re = None
    self.should_load_ = True

  def IsTranslation(self):
    return True

  def GetLang(self):
    return self.attrs['lang']

  def DisableLoading(self):
    self.should_load_ = False

  def MandatoryAttributes(self):
    return ['path', 'lang']

  def DefaultAttributes(self):
    return {
      'opera_path' : '', # empty means no extra path
    }

  def RunPostSubstitutionGatherer(self, debug=False):
    if not self.should_load_:
      return

    self._ParseXtb(self.GetInputPath(), debug)
    # Check if additional xtb is also provided
    if self.attrs['opera_path']:
      file_path = os.path.expandvars(self.attrs['opera_path'])
      if os.path.exists(self.ToRealPath(file_path)):
        self._ParseXtb(file_path, debug)
      else:
        print >> sys.stderr, 'NOTE: Specified opera_path %s for lang %s not found' % (file_path, self.attrs['lang'])

  def _ParseXtb(self, file_path, debug):
    root = self.GetRoot()
    defs = getattr(root, 'defines', {})
    target_platform = getattr(root, 'target_platform', '')
    xtb_file = open(self.ToRealPath(file_path))
    try:
      lang = xtb_reader.Parse(xtb_file,
                              self.UberClique().GenerateXtbParserCallback(
                                  self.attrs['lang'], debug=debug),
                              defs=defs,
                              target_platform=target_platform)
    except:
      print "Exception during parsing of %s" % file_path
      raise
    # Translation console uses non-standard language codes 'iw' and 'no' for
    # Hebrew and Norwegian Bokmal instead of 'he' and 'nb' used in Chrome.
    # Note that some Chrome's .grd still use 'no' instead of 'nb', but 'nb' is
    # always used for generated .pak files.
    # Bypass the assert for 'my-ZG' as OfA uses 'my' in the xtb files.
    ALTERNATIVE_LANG_CODE_MAP = { 'he': 'iw', 'nb': 'no', 'my-ZG': 'my' }
    assert (lang == self.attrs['lang'] or
            lang == ALTERNATIVE_LANG_CODE_MAP[self.attrs['lang']]), (
            'The XTB file you reference must contain messages in the language '
            'specified\nby the \'lang\' attribute.')

  def GetInputPath(self):
    return os.path.expandvars(self.attrs['path'])


class OutputNode(base.Node):
  '''An <output> element.'''

  def MandatoryAttributes(self):
    return ['filename', 'type']

  def DefaultAttributes(self):
    return {
      'lang' : '', # empty lang indicates all languages
      'language_section' : 'neutral', # defines a language neutral section
      'context' : '',
      'fallback_to_default_layout' : 'true',
    }

  def GetType(self):
    return self.attrs['type']

  def GetLanguage(self):
    '''Returns the language ID, default 'en'.'''
    return self.attrs['lang']

  def GetContext(self):
    return self.attrs['context']

  def GetFilename(self):
    return self.attrs['filename']

  def GetOutputFilename(self):
    path = None
    if hasattr(self, 'output_filename'):
      path = self.output_filename
    else:
      path = self.attrs['filename']
    return os.path.expandvars(path)

  def GetFallbackToDefaultLayout(self):
    return self.attrs['fallback_to_default_layout'].lower() == 'true'

  def _IsValidChild(self, child):
    return isinstance(child, EmitNode)

class EmitNode(base.ContentNode):
  ''' An <emit> element.'''

  def DefaultAttributes(self):
    return { 'emit_type' : 'prepend'}

  def GetEmitType(self):
    '''Returns the emit_type for this node. Default is 'append'.'''
    return self.attrs['emit_type']

