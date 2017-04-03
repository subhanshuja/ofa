// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_MIGRATION_OBML_FONT_INFO_H_
#define CHILL_BROWSER_MIGRATION_OBML_FONT_INFO_H_

namespace opera {

struct OBMLFontInfo {
  enum Family {
    SERIF = 0,
    SANS_SERIF,
    MONOSPACE
  };

  int obml_id;
  Family family;
  bool bold;
  bool italic;
  int size;
  int line_height;
};

}  // namespace opera

#endif  // CHILL_BROWSER_MIGRATION_OBML_FONT_INFO_H_
