// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "testing/gtest/include/gtest/gtest.h"

#include "mobile/common/url_color_util/url_fall_back_colors.h"

namespace mobile {

TEST(URLFallBackColors, TestNoInvalidAnswerInAllowedRange) {
  for (int i = 0; i < 32; i++) {
    EXPECT_NE(0, URLFallBackColors::GetFallBackColorNumber(i));
  }
}

TEST(URLFallBackColors, TestErrorReturnValueOnInvalidRange) {
  EXPECT_EQ(0, URLFallBackColors::GetFallBackColorNumber(-1));
  EXPECT_EQ(0, URLFallBackColors::GetFallBackColorNumber(-10));

  EXPECT_EQ(0, URLFallBackColors::GetFallBackColorNumber(32));
  EXPECT_EQ(0, URLFallBackColors::GetFallBackColorNumber(137));
}

TEST(URLFallBackColors, RandomColorValues) {
  EXPECT_EQ(0xFF3CA4DF, URLFallBackColors::GetFallBackColorNumber(7));
  EXPECT_EQ(0xFFF2B722, URLFallBackColors::GetFallBackColorNumber(30));
}

}  // namespace mobile
