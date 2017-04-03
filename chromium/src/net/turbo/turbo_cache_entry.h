// Copyright (c) 2013 Opera Software ASA. All rights reserved.

#ifndef NET_TURBO_TURBO_CACHE_ENTRY_H_
#define NET_TURBO_TURBO_CACHE_ENTRY_H_

#include <string>

namespace net {

class TurboCacheEntry {
 public:
  explicit TurboCacheEntry(int url_crc32)
      : url_crc32_(url_crc32) {
  }

  explicit TurboCacheEntry(const std::string& url);

  int url_crc32() const { return url_crc32_; }

  bool operator< (const TurboCacheEntry& other) const {
    return url_crc32_ < other.url_crc32_;
  }

  bool operator== (const TurboCacheEntry& other) const {
    return url_crc32_ == other.url_crc32_;
  }

 private:
  int url_crc32_;
};

}  // namespace net

#endif  // NET_TURBO_TURBO_CACHE_ENTRY_H_
