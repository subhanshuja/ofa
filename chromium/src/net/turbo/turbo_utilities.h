// Copyright (c) 2013 Opera Software ASA. All rights reserved.

#ifndef NET_TURBO_TURBO_UTILITIES_H_
#define NET_TURBO_TURBO_UTILITIES_H_

#include <string>

namespace net {

class TurboUtilities {
 public:
  static int hash_url(const std::string& url);
  static int hash_cookie(const std::string& cookie);
};

}  // namespace net

#endif  // NET_TURBO_TURBO_UTILITIES_H_
