// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "testing/gtest/include/gtest/gtest.h"

#include "url/gurl.h"

#include "mobile/common/url_color_util/url_color_table.h"
#include "mobile/common/url_color_util/url_fall_back_colors.h"
#include "mobile/common/url_color_util/url_hasher.h"

namespace mobile {

uint32_t magic_hash_transformer(uint32_t hash) {
  return (hash >> 27) & 0x1f;
}

uint8_t third_byte(uint32_t bytes) {
  return (bytes >> 16) & 0xff;
}

void ensure_color_matches_fallback_for_hash(const URLColorTable& color_table,
                                            const std::string& url_string) {
  const GURL url(url_string);

  uint32_t fg = 0, bg = 0, invert = 0;
  color_table.LookupColorForUrl(url, &fg, &bg, &invert);

  uint32_t hash = URLHasher::Hash(url);
  uint32_t color =
      URLFallBackColors::GetFallBackColorNumber(magic_hash_transformer(hash));

  if (third_byte(hash) < 51) {
    EXPECT_EQ(fg, color);
  } else {
    EXPECT_EQ(bg, color);
  }
}

TEST(URLColorTable, EmptyDataGivesFallBack) {
  URLColorTable color_table(NULL, 0);

  std::string url("http://ulv.no");

  ensure_color_matches_fallback_for_hash(color_table, url);
}

TEST(URLColorTable, OneExistingElementIsFound) {
  unsigned char hash_and_color[] = {0xae, 0x6d, 0x4e, 0x5e, 0xf2, 0xf1, 0xf0};
  URLColorTable color_table(hash_and_color, 7);

  uint32_t fg = 0, bg = 0, invert = 0;
  color_table.LookupColorForUrl(GURL("http://ulv.no"), &fg, &bg, &invert);

  EXPECT_EQ(0xfff0f1f2, bg);
}

TEST(URLColorTable, NonExistingElementIsNotFound) {
  unsigned char hash_and_color[] = {0xae, 0x6d, 0x4e, 0x5e, 0xf2, 0xf1, 0xf0};
  URLColorTable color_table(hash_and_color, 7);

  const GURL url("http://elg.no");
  uint32_t hash = URLHasher::Hash(url);
  uint32_t color =
      URLFallBackColors::GetFallBackColorNumber(magic_hash_transformer(hash));

  uint32_t fg = 0, bg = 0, invert = 0;
  color_table.LookupColorForUrl(url, &fg, &bg, &invert);

  if (third_byte(hash) < 51) {
    EXPECT_EQ(fg, color);
  } else {
    EXPECT_EQ(bg, color);
  }
}

TEST(URLColorTable, FindAllInSortedList) {
  unsigned char hash_and_color[] = {
      0xbc, 0xb0, 0x42, 0x2a, 0x35, 0x24, 0x13,  // <-- sau.no
      0xae, 0x6d, 0x4e, 0x5e, 0xf2, 0xf1, 0xf0,  // <-- ulv.no
      0x57, 0xf1, 0xac, 0x6d, 0xef, 0xcd, 0xab,  // <-- elg.no
  };

  URLColorTable color_table(hash_and_color,
                            sizeof(hash_and_color) / sizeof(hash_and_color[0]));

  uint32_t fg = 0, bg = 0, invert = 0;

  color_table.LookupColorForUrl(GURL("http://ulv.no"), &fg, &bg, &invert);
  EXPECT_EQ(0xfff0f1f2, bg);

  color_table.LookupColorForUrl(GURL("http://elg.no"), &fg, &bg, &invert);
  EXPECT_EQ(0xffabcdef, bg);

  color_table.LookupColorForUrl(GURL("http://sau.no"), &fg, &bg, &invert);
  EXPECT_EQ(0xff132435, bg);
}

TEST(URLColorTable, UnsortedTableResultsInFallback) {
  unsigned char hash_and_color[] = {
      0x57, 0xf1, 0xac, 0x6d, 0xef, 0xcd, 0xab,  // <-- elg.no
      0xae, 0x6d, 0x4e, 0x5e, 0xf2, 0xf1, 0xf0,  // <-- ulv.no
      0xbc, 0xb0, 0x42, 0x2a, 0x35, 0x24, 0x13,  // <-- sau.no
  };

  URLColorTable color_table(hash_and_color,
                            sizeof(hash_and_color) / sizeof(hash_and_color[0]));

  ensure_color_matches_fallback_for_hash(color_table, "http://elg.no");
  ensure_color_matches_fallback_for_hash(color_table, "http://sau.no");
}

}  // namespace mobile
