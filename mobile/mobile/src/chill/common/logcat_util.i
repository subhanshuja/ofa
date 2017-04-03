// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA. All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%include "std_string.i"

%{
#include "chill/common/logcat_util.h"
%}

namespace opera {

class OpLogcatArguments : public OpArguments {
 public:
  std::string results;
};

class LogcatUtil {
 public:
  LogcatUtil();

  void StartFetchData(size_t max_num_lines, OpRunnable callback);
};

}  // namespace opera
