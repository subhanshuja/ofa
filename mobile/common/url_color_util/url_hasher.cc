// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/url_color_util/url_hasher.h"

#include <string.h>

#include "url/gurl.h"

namespace mobile {
namespace {

#if defined(_MSC_VER)
# define FORCE_INLINE  __forceinline
#else // defined(_MSC_VER)
# define FORCE_INLINE inline __attribute__((always_inline))
#endif // !defined(_MSC_VER)

/* Logical shift right of x by n */
FORCE_INLINE int32_t lsr(int32_t x, int32_t n) {
  return (int32_t) ((uint32_t) x >> n);
}

/*
  Special implementation of murmurhash3 to result in the same hash values as
  the javascript port we use to generate the color LUT.
*/

int32_t MurmurHash3_signed_int32(const uint8_t *input, size_t length,
  int32_t seed) {
  int32_t h1 = seed;
  size_t fourBytesLength = length - (length & 3);
  int32_t C1 = 0xcc9e2d51;
  int32_t C2 = 0x1b873593;
  int32_t k1 = 0;
  for (size_t i = 0; i < fourBytesLength >> 2; ) {
      k1 = ((int32_t*) input)[i++];
      k1 = k1 * C1;
      k1 = k1 << 15 | lsr(k1, 17);
      k1 = k1 * C2;
      h1 ^= k1;
      h1 = h1 << 13 | lsr(h1, 19);
      h1 = h1 * 5 + 0xe6546b64;
  }
  k1 = 0;
  switch (length & 3) {
      case 3:
          k1 ^= input[fourBytesLength + 2] << 16;
      case 2:
          k1 ^= input[fourBytesLength + 1] << 8;
      case 1: k1 ^= input[fourBytesLength];
  }
  k1 = k1 * C1;
  k1 = k1 << 15 | lsr(k1, 17);
  k1 = k1 * C2;
  h1 ^= k1;
  h1 ^= length;
  h1 ^= lsr(h1, 16);
  h1 = h1 * 0x85ebca6b;
  h1 ^= h1 >> 13;
  h1 = h1 * 0xc2b2ae35;
  h1 ^= h1 >> 16;
  return h1;
}

// This seed is used on the server side and must be shared with all clients.
// It's expected to never change during the life time of the product.
const uint32_t kMurmurSeed = 0xc3d2e1f0;

const char* kWwwPrefix = "www.";

std::string extract_stripped_domain(const GURL& url) {
  std::string domain = url.host();
  size_t pos = domain.find(kWwwPrefix);

  if (pos == 0) {
    return domain.substr(strlen(kWwwPrefix));
  }

  return domain;
}

}  // namespace

/* static */
uint32_t URLHasher::Hash(const GURL& url) {
  std::string domain = extract_stripped_domain(url);

  // Note: The javascript code used to generate the hashes, do so on the
  // punycode encoded version of the url. There's also some extra conversion
  // of charcodes. If you experience misses, this needs to be fixed too.

  return MurmurHash3_signed_int32(
    (const uint8_t *) domain.c_str(), domain.length(), kMurmurSeed);
}

}  // namespace mobile
