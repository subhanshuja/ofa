// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_COMMON_LOGCAT_UTIL_H_
#define CHILL_COMMON_LOGCAT_UTIL_H_

#include <string>

#include "base/callback.h"
#include "common/swig_utils/op_arguments.h"
#include "common/swig_utils/op_runnable.h"

namespace opera {

typedef OpRunnable LogcatDataCallback;

class OpLogcatArguments : public OpArguments {
 public:
  std::string results;

 private:
  SWIG_CLASS_NAME
};

class LogcatUtil {
 public:
  LogcatUtil() {}

  // Set max_num_lines=0 to fetch all lines
  void StartFetchData(size_t max_num_lines, LogcatDataCallback callback);

 private:
  LogcatDataCallback callback_;
  size_t max_num_lines_;

  void FetchData();
  void EndFetchData(const std::string data);

  DISALLOW_COPY_AND_ASSIGN(LogcatUtil);
};

}  // namespace opera

#endif  // CHILL_COMMON_LOGCAT_UTIL_H_
