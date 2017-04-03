// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_COLOR_UTIL_COLOR_UTIL_H_
#define MOBILE_COMMON_COLOR_UTIL_COLOR_UTIL_H_

#include <vector>

#include <stdint.h>
#include <stdlib.h>

class GURL;

namespace mobile {

/**
 * Translates URL domains to colors using a given table or a set of fallback
 * colors.
 *
 * The data provided to the constructor is in the implicit form of a list of
 * pairs of URL hash values and RGB triplets, each item being 7 bytes long (4
 * bytes hash + 3 bytes RGB). The hash value is encoded with the most
 * significant byte first, and the colors are encoded in RGB order.
 *
 * The hash algorithm used is MurmurHash3_32.
 *
 * The color values are returned in int values encoded as ARGB (one byte per
 * color) with the top byte set to 0xff. Failure to find a value (a programming
 * error!) will show up as the color being 0.
 */
class URLColorTable {
 public:
  /**
   * Create a lookup table using the data_table data in the format described in
   * the class documentation. The table length should be a multiple of 7.
   */
  URLColorTable(const uint8_t* data_table, size_t data_table_length);
  ~URLColorTable();

  bool LookupColorForUrl(const GURL& url,
                         uint32_t* foreground_color,
                         uint32_t* background_color,
                         uint32_t* invert) const;

 private:
  bool FindColor(uint32_t hash, uint32_t* color) const;
  uint32_t GetHashForItemWithIndex(size_t index) const;
  uint32_t GetColorForItemWithIndex(size_t index) const;

  const std::vector<uint8_t> color_table_;
};

}  // namespace mobile

#endif  // MOBILE_COMMON_COLOR_UTIL_COLOR_UTIL_H_
