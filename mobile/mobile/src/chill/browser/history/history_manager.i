// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "common/swig_utils/op_arguments.h"
#include "chill/browser/history/history_manager.h"
%}

namespace opera {

class HistoryManager {
 public:
  HistoryManager();
  virtual ~HistoryManager();

  void SetCallback(OpRunnable callback);
  void Observe(bool enabled);
  void ForceUpdate();
  void Clear(OpRunnable callback);
  void Remove(GURL url);
  void GetFirstVisitTime(GURL url, OpRunnable callback);
};

class OpGetFirstVisitTimeArguments : public OpArguments {
 public:
 bool success;
 int visits_count;
 base::Time first_visit_time;
};

}  // namespace opera
