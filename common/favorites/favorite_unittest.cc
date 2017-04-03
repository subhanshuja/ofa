// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/favorites/favorite.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace opera{
namespace {

const char kURL[] = "http://host.com";
const char kPartnerID[] = "pampam";

TEST(FavoriteSiteTest, NavigateURLEqualsDisplayURLWithoutRedirect) {
  FavoriteData data;
  data.url = GURL(kURL);

  FavoriteSite site0("", data);
  EXPECT_EQ(GURL(kURL), site0.display_url());
  EXPECT_EQ(GURL(kURL), site0.navigate_url());

  data.partner_id = kPartnerID;
  FavoriteSite site1("", data);
  EXPECT_EQ(GURL(kURL), site1.display_url());
  EXPECT_EQ(GURL(kURL), site1.navigate_url());
}

TEST(FavoriteSiteTest, NavigateURLBasedOnPartnerIDWithRedirect) {
  FavoriteData data;
  data.url = GURL(kURL);
  data.partner_id = kPartnerID;
  data.redirect = true;
  FavoriteSite site("", data);
  EXPECT_EQ(GURL(kURL), site.display_url());
  EXPECT_NE(std::string::npos, site.navigate_url().spec().find(kPartnerID));
}

}  // namespace
}  // namespace opera
