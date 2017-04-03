// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_COOKIES_COOKIE_DOMAIN_H_
#define COMMON_MIGRATION_COOKIES_COOKIE_DOMAIN_H_
#include <stdint.h>
#include <string>
#include <vector>
#include <map>

#include "common/migration/cookies/imported_cookie.h"
#include "common/migration/cookies/path_component.h"
#include "base/callback.h"
#include "net/base/net_export.h"

namespace net {
// Forward declaration, defined in net/cookies/cookie_store.h
class NET_EXPORT cookie_store;
}

namespace opera {
namespace common {
namespace migration {

class DataStreamReader;

/** Represents a cookie domain record, as described on
 * http://www.opera.com/docs/operafiles/#cookies
 *
 * A cookie domain may contain cookies, path components and
 * subdomains (recursively).
 * Example, the "www.opera.com" address is represented as:
 *
 * @code
 * [CookieDomain: com]
 *  [CookieDomain: opera] // Subdomain of com
 *  [ImportedCookie] // Cookie for opera.com
 *   [CookieDomain: www] // Subdomain of opera.com
 *    [PathComponent: some]
 *    [ImportedCookie] // Cookie for www.opera.com/some
 *     [PathComponent: path] // Subpath of some
 *     [ImportedCookie] // Cookie for www.opera.com/some/path
 * @endcode
 */
class CookieDomain {
 public:
  CookieDomain();
  CookieDomain(const CookieDomain&);
  ~CookieDomain();

  bool Parse(DataStreamReader* reader);
  void StoreCookies(
      const std::string& parent_domain,
      const CookieCallback& on_new_cookie_callback) const;

 private:
  void RegisterTagHandlers();
  bool ParseCookie(DataStreamReader* reader);
  bool ParsePath(DataStreamReader* reader);
  bool ParseSubdomain(DataStreamReader* reader);

  typedef base::Callback<bool(DataStreamReader*)> TagHandler; // NOLINT
  typedef std::map<uint8_t, TagHandler> TagHandlersMap;
  std::map<uint8_t, TagHandler> tag_handlers_;
  std::string domain_name_;
  std::vector<ImportedCookie> cookies_;
  std::vector<PathComponent> path_components_;
  std::vector<CookieDomain> subdomains_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_COOKIES_COOKIE_DOMAIN_H_
