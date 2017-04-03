// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_COOKIES_PATH_COMPONENT_H_
#define COMMON_MIGRATION_COOKIES_PATH_COMPONENT_H_
#include <string>
#include <vector>

#include "common/migration/cookies/imported_cookie.h"
#include "net/base/net_export.h"

namespace net {
// Forward declaration, defined in net/cookies/cookie_store.h
class NET_EXPORT cookie_store;
}

namespace opera {
namespace common {
namespace migration {

class DataStreamReader;

/** Represents a path component extracted from a cookie data file.
 *
 * A path component may contain cookies and subcomponents (recursively).
 * Example, the "www.opera.com/some/path" address is represented as:
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
 *
 * @ref http://www.opera.com/docs/operafiles/#cookies
 */
class PathComponent {
 public:
  PathComponent();
  PathComponent(const PathComponent&);
  ~PathComponent();
  bool Parse(DataStreamReader* reader);
  void StoreCookies(
      const std::string& parent_path,
      const CookieCallback& on_new_cookie_callback) const;

 private:
  std::string name_;
  std::vector<ImportedCookie> cookies_;
  std::vector<PathComponent> subcomponents_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_COOKIES_PATH_COMPONENT_H_
