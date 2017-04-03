// Copyright (c) 2013 Opera Software ASA. All rights reserved.

#include "net/turbo/turbo_utilities.h"
#include "third_party/zlib/zlib.h"

namespace net {

namespace {

int hash_crc32(const std::string& string) {
  return crc32(
      0L, reinterpret_cast<const Bytef*>(string.c_str()), string.size());
}

}

int TurboUtilities::hash_url(const std::string& url) {
  return hash_crc32(url);
}

int TurboUtilities::hash_cookie(const std::string& cookie) {
  return hash_crc32(cookie);
}

}  // namespace net
