// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/url_color_util/url_color_table.h"

#include "base/logging.h"

#include "mobile/common/url_color_util/url_fall_back_colors.h"
#include "mobile/common/url_color_util/url_hasher.h"

#if 0
# include "url/gurl.h"
# include <android/log.h>
# define URL_COLOR_TBL_LOG(fmt, ...) \
  __android_log_print(ANDROID_LOG_DEBUG, "url_color_table", fmt, ## __VA_ARGS__)
#else
# define URL_COLOR_TBL_LOG(fmt, ...) (void)0
#endif

namespace {

const size_t kObjectSize = 7;
const size_t kColorOffset = 4;

const uint32_t kWhiteColor = 0xffffffff;

bool should_invert_colors(uint32_t hash) {
  // Magic condition from ANDUI-4878
  return ((hash >> 16) & 0xff) < 51;
}

}  // namespace

namespace mobile {

URLColorTable::URLColorTable(const uint8_t* data_table,
                             size_t data_table_length)
    : color_table_(data_table, &data_table[data_table_length]) {
  DCHECK(data_table_length % kObjectSize == 0);
}

URLColorTable::~URLColorTable() {}

bool URLColorTable::LookupColorForUrl(const GURL& url,
                                      uint32_t* foreground_color,
                                      uint32_t* background_color,
                                      uint32_t* invert) const {
  const uint32_t hash = URLHasher::Hash(url);

  uint32_t color = 0;
  bool found = FindColor(hash, &color);
  if (!found) {
    color = URLFallBackColors::GetFallBackColorNumber(hash >> 27);
  }

  DCHECK(color != 0);
  *invert = should_invert_colors(hash) ? 1 : 0;
  *foreground_color = *invert ? color : kWhiteColor;
  *background_color = *invert ? kWhiteColor : color;
  URL_COLOR_TBL_LOG("%s: %s [hash: 0x%08x], bg: 0x%08x, fg: 0x%08x, invert: %d",
    found ? "Found" : "Not found",
    url.spec().c_str(),
    hash,
    *background_color, *foreground_color,
    should_invert_colors(hash) ? 1 : 0);
  return found;
}

bool URLColorTable::FindColor(uint32_t hash, uint32_t* color) const {
  for (size_t lower = 0, upper = color_table_.size() / kObjectSize;
       lower < upper;) {
    size_t middle_element = (lower + upper) / 2;
    uint32_t middle_element_hash = GetHashForItemWithIndex(middle_element);

    if (hash <= middle_element_hash) {
      if (hash == middle_element_hash) {
        *color = GetColorForItemWithIndex(middle_element);
        return true;
      }

      upper = middle_element;
    } else {
      lower = middle_element + 1;
    }
  }
  return false;
}

uint32_t URLColorTable::GetHashForItemWithIndex(size_t index) const {
  return (color_table_[index * kObjectSize + 3] << 24) |
         (color_table_[index * kObjectSize + 2] << 16) |
         (color_table_[index * kObjectSize + 1] <<  8) |
         (color_table_[index * kObjectSize + 0] <<  0);
}

uint32_t URLColorTable::GetColorForItemWithIndex(size_t index) const {
  return 0xff000000 |
         (color_table_[index * kObjectSize + kColorOffset + 2] << 16) |
         (color_table_[index * kObjectSize + kColorOffset + 1] <<  8) |
         (color_table_[index * kObjectSize + kColorOffset + 0] <<  0);
}

}  // namespace mobile
