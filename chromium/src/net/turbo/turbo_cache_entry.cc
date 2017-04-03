// Copyright (c) 2013 Opera Software ASA. All rights reserved.

#include "net/turbo/turbo_cache_entry.h"
#include "net/turbo/turbo_utilities.h"

namespace net {

TurboCacheEntry::TurboCacheEntry(const std::string& url)
    : url_crc32_(TurboUtilities::hash_url(url)) {
}

}  // namespace net
