// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/account/account_util.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace opera {
namespace {

using ::testing::_;
using ::testing::Return;

class AccountUtilTest : public ::testing::Test {
 public:
  AccountUtilTest() {}
  ~AccountUtilTest() override {}

 protected:
  const base::TimeDelta kZero = base::TimeDelta();
  const base::TimeDelta k33d = base::TimeDelta::FromDays(33);
  const base::TimeDelta k3d = base::TimeDelta::FromDays(3);
  const base::TimeDelta k4h = base::TimeDelta::FromHours(4);
  const base::TimeDelta k5m = base::TimeDelta::FromMinutes(5);
  const base::TimeDelta k6s = base::TimeDelta::FromSeconds(6);
  const base::TimeDelta k7ms = base::TimeDelta::FromMilliseconds(7);
  const base::TimeDelta k8us = base::TimeDelta::FromMicroseconds(8);

  const base::TimeDelta k3d4h = k3d + k4h;
  const base::TimeDelta k3d4h5m = k3d4h + k5m;
  const base::TimeDelta k3d4h5m6s = k3d4h5m + k6s;
  const base::TimeDelta k3d4h5m6s7ms = k3d4h5m6s + k7ms;
  const base::TimeDelta k3d4h5m6s7ms8us = k3d4h5m6s7ms + k8us;
};

TEST_F(AccountUtilTest, TimeDeltaToStringPrecisionDays) {
  ASSERT_EQ("0 d", account_util::TimeDeltaToString(
                       kZero, account_util::PRECISION_DAYS));

  ASSERT_EQ("33 d", account_util::TimeDeltaToString(
                        k33d, account_util::PRECISION_DAYS));
  ASSERT_EQ("3 d",
            account_util::TimeDeltaToString(k3d, account_util::PRECISION_DAYS));
  ASSERT_EQ("0 d",
            account_util::TimeDeltaToString(k4h, account_util::PRECISION_DAYS));
  ASSERT_EQ("0 d",
            account_util::TimeDeltaToString(k5m, account_util::PRECISION_DAYS));
  ASSERT_EQ("0 d",
            account_util::TimeDeltaToString(k6s, account_util::PRECISION_DAYS));
  ASSERT_EQ("0 d", account_util::TimeDeltaToString(
                       k7ms, account_util::PRECISION_DAYS));
  ASSERT_EQ("0 d", account_util::TimeDeltaToString(
                       k8us, account_util::PRECISION_DAYS));

  ASSERT_EQ("3 d", account_util::TimeDeltaToString(
                       k3d4h, account_util::PRECISION_DAYS));
  ASSERT_EQ("3 d", account_util::TimeDeltaToString(
                       k3d4h5m, account_util::PRECISION_DAYS));
  ASSERT_EQ("3 d", account_util::TimeDeltaToString(
                       k3d4h5m6s, account_util::PRECISION_DAYS));
  ASSERT_EQ("3 d", account_util::TimeDeltaToString(
                       k3d4h5m6s7ms, account_util::PRECISION_DAYS));
  ASSERT_EQ("3 d", account_util::TimeDeltaToString(
                       k3d4h5m6s7ms8us, account_util::PRECISION_DAYS));
}

TEST_F(AccountUtilTest, TimeDeltaToStringPrecisionHours) {
  ASSERT_EQ("0 h", account_util::TimeDeltaToString(
                       kZero, account_util::PRECISION_HOURS));

  ASSERT_EQ("33 d", account_util::TimeDeltaToString(
                        k33d, account_util::PRECISION_HOURS));
  ASSERT_EQ("3 d", account_util::TimeDeltaToString(
                       k3d, account_util::PRECISION_HOURS));
  ASSERT_EQ("4 h", account_util::TimeDeltaToString(
                       k4h, account_util::PRECISION_HOURS));
  ASSERT_EQ("0 h", account_util::TimeDeltaToString(
                       k5m, account_util::PRECISION_HOURS));
  ASSERT_EQ("0 h", account_util::TimeDeltaToString(
                       k6s, account_util::PRECISION_HOURS));
  ASSERT_EQ("0 h", account_util::TimeDeltaToString(
                       k7ms, account_util::PRECISION_HOURS));
  ASSERT_EQ("0 h", account_util::TimeDeltaToString(
                       k8us, account_util::PRECISION_HOURS));

  ASSERT_EQ("3 d 4 h", account_util::TimeDeltaToString(
                           k3d4h, account_util::PRECISION_HOURS));
  ASSERT_EQ("3 d 4 h", account_util::TimeDeltaToString(
                           k3d4h5m, account_util::PRECISION_HOURS));
  ASSERT_EQ("3 d 4 h", account_util::TimeDeltaToString(
                           k3d4h5m6s, account_util::PRECISION_HOURS));
  ASSERT_EQ("3 d 4 h", account_util::TimeDeltaToString(
                           k3d4h5m6s7ms, account_util::PRECISION_HOURS));
  ASSERT_EQ("3 d 4 h", account_util::TimeDeltaToString(
                           k3d4h5m6s7ms8us, account_util::PRECISION_HOURS));
}

TEST_F(AccountUtilTest, TimeDeltaToStringPrecisionMinutes) {
  ASSERT_EQ("0 min", account_util::TimeDeltaToString(
                         kZero, account_util::PRECISION_MINUTES));

  ASSERT_EQ("33 d", account_util::TimeDeltaToString(
                        k33d, account_util::PRECISION_MINUTES));
  ASSERT_EQ("3 d", account_util::TimeDeltaToString(
                       k3d, account_util::PRECISION_MINUTES));
  ASSERT_EQ("4 h", account_util::TimeDeltaToString(
                       k4h, account_util::PRECISION_MINUTES));
  ASSERT_EQ("5 min", account_util::TimeDeltaToString(
                         k5m, account_util::PRECISION_MINUTES));
  ASSERT_EQ("0 min", account_util::TimeDeltaToString(
                         k6s, account_util::PRECISION_MINUTES));
  ASSERT_EQ("0 min", account_util::TimeDeltaToString(
                         k7ms, account_util::PRECISION_MINUTES));
  ASSERT_EQ("0 min", account_util::TimeDeltaToString(
                         k8us, account_util::PRECISION_MINUTES));

  ASSERT_EQ("3 d 4 h", account_util::TimeDeltaToString(
                           k3d4h, account_util::PRECISION_MINUTES));
  ASSERT_EQ("3 d 4 h 5 min", account_util::TimeDeltaToString(
                                 k3d4h5m, account_util::PRECISION_MINUTES));
  ASSERT_EQ("3 d 4 h 5 min", account_util::TimeDeltaToString(
                                 k3d4h5m6s, account_util::PRECISION_MINUTES));
  ASSERT_EQ("3 d 4 h 5 min",
            account_util::TimeDeltaToString(k3d4h5m6s7ms,
                                            account_util::PRECISION_MINUTES));
  ASSERT_EQ("3 d 4 h 5 min",
            account_util::TimeDeltaToString(k3d4h5m6s7ms8us,
                                            account_util::PRECISION_MINUTES));
}

TEST_F(AccountUtilTest, TimeDeltaToStringPrecisionSeconds) {
  ASSERT_EQ("0 s", account_util::TimeDeltaToString(
                       kZero, account_util::PRECISION_SECONDS));

  ASSERT_EQ("33 d", account_util::TimeDeltaToString(
                        k33d, account_util::PRECISION_SECONDS));
  ASSERT_EQ("3 d", account_util::TimeDeltaToString(
                       k3d, account_util::PRECISION_SECONDS));
  ASSERT_EQ("4 h", account_util::TimeDeltaToString(
                       k4h, account_util::PRECISION_SECONDS));
  ASSERT_EQ("5 min", account_util::TimeDeltaToString(
                         k5m, account_util::PRECISION_SECONDS));
  ASSERT_EQ("6 s", account_util::TimeDeltaToString(
                       k6s, account_util::PRECISION_SECONDS));
  ASSERT_EQ("0 s", account_util::TimeDeltaToString(
                       k7ms, account_util::PRECISION_SECONDS));
  ASSERT_EQ("0 s", account_util::TimeDeltaToString(
                       k8us, account_util::PRECISION_SECONDS));

  ASSERT_EQ("3 d 4 h", account_util::TimeDeltaToString(
                           k3d4h, account_util::PRECISION_SECONDS));
  ASSERT_EQ("3 d 4 h 5 min", account_util::TimeDeltaToString(
                                 k3d4h5m, account_util::PRECISION_SECONDS));
  ASSERT_EQ("3 d 4 h 5 min 6 s",
            account_util::TimeDeltaToString(k3d4h5m6s,
                                            account_util::PRECISION_SECONDS));
  ASSERT_EQ("3 d 4 h 5 min 6 s",
            account_util::TimeDeltaToString(k3d4h5m6s7ms,
                                            account_util::PRECISION_SECONDS));
  ASSERT_EQ("3 d 4 h 5 min 6 s",
            account_util::TimeDeltaToString(k3d4h5m6s7ms8us,
                                            account_util::PRECISION_SECONDS));
}

TEST_F(AccountUtilTest, TimeDeltaToStringPrecisionMilliseconds) {
  ASSERT_EQ("0 ms", account_util::TimeDeltaToString(
                        kZero, account_util::PRECISION_MILLISECONDS));

  ASSERT_EQ("33 d", account_util::TimeDeltaToString(
                        k33d, account_util::PRECISION_MILLISECONDS));
  ASSERT_EQ("3 d", account_util::TimeDeltaToString(
                       k3d, account_util::PRECISION_MILLISECONDS));
  ASSERT_EQ("4 h", account_util::TimeDeltaToString(
                       k4h, account_util::PRECISION_MILLISECONDS));
  ASSERT_EQ("5 min", account_util::TimeDeltaToString(
                         k5m, account_util::PRECISION_MILLISECONDS));
  ASSERT_EQ("6 s", account_util::TimeDeltaToString(
                       k6s, account_util::PRECISION_MILLISECONDS));
  ASSERT_EQ("7 ms", account_util::TimeDeltaToString(
                        k7ms, account_util::PRECISION_MILLISECONDS));
  ASSERT_EQ("0 ms", account_util::TimeDeltaToString(
                        k8us, account_util::PRECISION_MILLISECONDS));

  ASSERT_EQ("3 d 4 h", account_util::TimeDeltaToString(
                           k3d4h, account_util::PRECISION_MILLISECONDS));
  ASSERT_EQ("3 d 4 h 5 min",
            account_util::TimeDeltaToString(
                k3d4h5m, account_util::PRECISION_MILLISECONDS));
  ASSERT_EQ("3 d 4 h 5 min 6 s",
            account_util::TimeDeltaToString(
                k3d4h5m6s, account_util::PRECISION_MILLISECONDS));
  ASSERT_EQ("3 d 4 h 5 min 6 s 7 ms",
            account_util::TimeDeltaToString(
                k3d4h5m6s7ms, account_util::PRECISION_MILLISECONDS));
  ASSERT_EQ("3 d 4 h 5 min 6 s 7 ms",
            account_util::TimeDeltaToString(
                k3d4h5m6s7ms8us, account_util::PRECISION_MILLISECONDS));
}

TEST_F(AccountUtilTest, TimeDeltaToStringPrecisionMicroseconds) {
  ASSERT_EQ("0 us", account_util::TimeDeltaToString(
                        kZero, account_util::PRECISION_MICROSECONDS));

  ASSERT_EQ("33 d", account_util::TimeDeltaToString(
                        k33d, account_util::PRECISION_MICROSECONDS));
  ASSERT_EQ("3 d", account_util::TimeDeltaToString(
                       k3d, account_util::PRECISION_MICROSECONDS));
  ASSERT_EQ("4 h", account_util::TimeDeltaToString(
                       k4h, account_util::PRECISION_MICROSECONDS));
  ASSERT_EQ("5 min", account_util::TimeDeltaToString(
                         k5m, account_util::PRECISION_MICROSECONDS));
  ASSERT_EQ("6 s", account_util::TimeDeltaToString(
                       k6s, account_util::PRECISION_MICROSECONDS));
  ASSERT_EQ("7 ms", account_util::TimeDeltaToString(
                        k7ms, account_util::PRECISION_MICROSECONDS));
  ASSERT_EQ("8 us", account_util::TimeDeltaToString(
                        k8us, account_util::PRECISION_MICROSECONDS));

  ASSERT_EQ("3 d 4 h", account_util::TimeDeltaToString(
                           k3d4h, account_util::PRECISION_MICROSECONDS));
  ASSERT_EQ("3 d 4 h 5 min",
            account_util::TimeDeltaToString(
                k3d4h5m, account_util::PRECISION_MICROSECONDS));
  ASSERT_EQ("3 d 4 h 5 min 6 s",
            account_util::TimeDeltaToString(
                k3d4h5m6s, account_util::PRECISION_MICROSECONDS));
  ASSERT_EQ("3 d 4 h 5 min 6 s 7 ms",
            account_util::TimeDeltaToString(
                k3d4h5m6s7ms, account_util::PRECISION_MICROSECONDS));
  ASSERT_EQ("3 d 4 h 5 min 6 s 7 ms 8 us",
            account_util::TimeDeltaToString(
                k3d4h5m6s7ms8us, account_util::PRECISION_MICROSECONDS));
}

}  // namespace
}  // namespace opera
