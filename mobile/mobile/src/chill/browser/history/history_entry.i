// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/history/history_entry.h"

#include <string>
#include <vector>

#include "url/gurl.h"

#include "common/swig_utils/op_arguments.h"
%}

VECTOR(HistoryEntryList, opera::HistoryEntry);

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
  std::vector<opera::HistoryEntry> entries;
};


}  // namespace opera
