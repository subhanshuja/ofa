// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/account/time_skew_resolver.h"

namespace opera {

class TimeSkewResolverMock : public TimeSkewResolver {
 public:
  TimeSkewResolverMock();
  ~TimeSkewResolverMock() override;

  void RequestTimeSkew(ResultCallback callback) override;

  void SetResultValue(ResultValue value);

 private:
  std::unique_ptr<ResultValue> value_;
  ResultCallback callback_;
};

}  // namespace opera
