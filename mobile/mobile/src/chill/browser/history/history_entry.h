// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_HISTORY_HISTORY_ENTRY_H_
#define COMMON_HISTORY_HISTORY_ENTRY_H_

#include <string>
#include <vector>

#include "chill/browser/history/history_entry.h"
#include "common/swig_utils/op_arguments.h"
#include "url/gurl.h"

namespace opera {

class HistoryEntry {
 public:
  int64_t id;
  GURL url;
  std::string title;
  base::Time visitTime;
};

class OpHistoryArguments : public OpArguments {
 public:
  std::vector<HistoryEntry> entries;
 private:
  SWIG_CLASS_NAME
};

}  // namespace opera

#endif  // COMMON_HISTORY_HISTORY_ENTRY_H_
