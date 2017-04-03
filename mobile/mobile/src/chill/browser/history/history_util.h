// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_HISTORY_HISTORY_UTIL_H_
#define COMMON_HISTORY_HISTORY_UTIL_H_

#include <vector>

namespace opera {

class HistoryEntry;

class HistoryUtil {
 public:
  static void ExportEntries(std::vector<opera::HistoryEntry> entries);
};

}  // namespace opera

#endif  // COMMON_HISTORY_HISTORY_UTIL_H_
