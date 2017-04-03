// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "testing/gtest/include/gtest/gtest.h"

#include "url/gurl.h"

#include "mobile/common/url_color_util/url_hasher.h"

namespace mobile {

TEST(UrlHasher, MatchSimpleUrl) {
  EXPECT_EQ(0x6dacf157, URLHasher::Hash(GURL("http://elg.no")));
}

TEST(UrlHasher, StripWwwPrefix) {
  EXPECT_EQ(0x6dacf157, URLHasher::Hash(GURL("http://www.elg.no")));
}

TEST(UrlHasher, StripMore) {
  EXPECT_EQ(0x60f7c92f, URLHasher::Hash(GURL("https://www.youtube.com/watch?v=h8bcrFLaS7g")));
}

}  // namespace mobile
