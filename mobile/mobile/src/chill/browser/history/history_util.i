// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/history/history_util.h"

#include "chill/browser/history/history_entry.h"
%}

namespace opera {

class HistoryUtil {
 public:
  static void ExportEntries(std::vector<opera::HistoryEntry>);
};

}  // namespace opera
