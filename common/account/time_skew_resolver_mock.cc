// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/account/time_skew_resolver_mock.h"

namespace opera {

TimeSkewResolverMock::TimeSkewResolverMock()
    : value_(new ResultValue(QueryResult::TIME_OK, 0, "")), callback_() {
}

TimeSkewResolverMock::~TimeSkewResolverMock() {
}

void TimeSkewResolverMock::RequestTimeSkew(ResultCallback callback) {
  callback.Run(*value_);
}

void TimeSkewResolverMock::SetResultValue(ResultValue value) {
  value_.reset(new ResultValue(value));
}

}  // namespace opera
