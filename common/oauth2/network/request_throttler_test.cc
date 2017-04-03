// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/network/request_throttler.h"

#include <memory>

#include "base/test/test_mock_time_task_runner.h"
#include "base/time/tick_clock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace opera {
namespace oauth2 {

namespace {
using ::testing::_;
using ::testing::Return;

class NetworkRequestThrottlerTest : public ::testing::Test {
 public:
  NetworkRequestThrottlerTest()
      : mock_time_task_runner_(new base::TestMockTimeTaskRunner()),
        mock_tick_clock_(mock_time_task_runner_->GetMockTickClock()) {}

 protected:
  const std::string kKey1 = "key1";
  const std::string kKey2 = "key2";

  void AdvanceByMs(int ms) {
    mock_time_task_runner_->FastForwardBy(
        base::TimeDelta::FromMilliseconds(ms));
  }

  scoped_refptr<base::TestMockTimeTaskRunner> mock_time_task_runner_;
  std::unique_ptr<base::TickClock> mock_tick_clock_;
};

}  // namespace

TEST_F(NetworkRequestThrottlerTest, EntersThrottling) {
  RequestThrottler throttler(mock_tick_clock_.get());

  base::TimeDelta delay1_1 = throttler.GetAndUpdateRequestDelay("key1");
  base::TimeDelta delay2_1 = throttler.GetAndUpdateRequestDelay("key2");
  base::TimeDelta delay1_2 = throttler.GetAndUpdateRequestDelay("key1");
  base::TimeDelta delay1_3 = throttler.GetAndUpdateRequestDelay("key1");
  base::TimeDelta delay2_2 = throttler.GetAndUpdateRequestDelay("key2");

  EXPECT_EQ(base::TimeDelta(), delay1_1);
  EXPECT_EQ(base::TimeDelta(), delay2_1);
  EXPECT_GT(delay1_2, delay1_1);
  EXPECT_GT(delay1_3, delay1_2);
  EXPECT_GT(delay2_2, delay2_1);
}

TEST_F(NetworkRequestThrottlerTest, AvoidsThrottling) {
  RequestThrottler throttler(mock_tick_clock_.get());

  base::TimeDelta delay1_1 = throttler.GetAndUpdateRequestDelay("key1");
  base::TimeDelta delay2_1 = throttler.GetAndUpdateRequestDelay("key2");
  AdvanceByMs(3000);
  base::TimeDelta delay1_2 = throttler.GetAndUpdateRequestDelay("key1");
  base::TimeDelta delay2_2 = throttler.GetAndUpdateRequestDelay("key2");
  AdvanceByMs(3000);
  base::TimeDelta delay1_3 = throttler.GetAndUpdateRequestDelay("key1");
  base::TimeDelta delay2_3 = throttler.GetAndUpdateRequestDelay("key2");

  EXPECT_EQ(base::TimeDelta(), delay1_1);
  EXPECT_EQ(base::TimeDelta(), delay2_1);
  EXPECT_EQ(base::TimeDelta(), delay1_2);
  EXPECT_EQ(base::TimeDelta(), delay2_2);
  EXPECT_EQ(base::TimeDelta(), delay1_3);
  EXPECT_EQ(base::TimeDelta(), delay2_3);
}

}  // namespace oauth2
}  // namespace opera
