// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "base/time/time.h"
%}

namespace base {
class Time {
 public:
  Time();
  int64_t ToJavaTime() const;
};

%extend Time {
 public:
  static Time FromJavaTime(int64_t time) {
    return base::Time::FromJsTime(static_cast<double>(time));
  }
};

}
