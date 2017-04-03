// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_COOKIES_IMPORTED_COOKIE_H_
#define COMMON_MIGRATION_COOKIES_IMPORTED_COOKIE_H_
#include <stdint.h>
#include <ctime>
#include <map>
#include <string>

#include "net/base/net_export.h"
#include "base/callback.h"
#include "common/migration/cookies/cookie_callback.h"

namespace net {
// Forward declaration, defined in net/cookies/cookie_store.h
class NET_EXPORT CookieStore;
}

namespace opera {
namespace common {
namespace migration {

class DataStreamReader;

/** Represents a single cookie imported from Presto's cookie file.
 * A cookie knows its fields but not the URL it's assigned to.
 *
 * @ref http://www.opera.com/docs/operafiles/#cookies
 * @ref http://www.ietf.org/rfc/rfc2965.txt
 */
class ImportedCookie {
 public:
  ImportedCookie();
  ImportedCookie(const ImportedCookie&);
  ~ImportedCookie();
  bool Parse(DataStreamReader* reader);

  /** Creates a cookie line and stores it, assigning to the given url, inside
   * the cookie store.
   */
  void StoreSelf(
      const std::string& url,
      const CookieCallback& on_new_cookie_callback) const;

  /** Returns a cookie line, without "Set-Cookie:".
   *
   * Follows RFC 2965.
   */
  std::string CreateCookieLine() const;

 private:
  std::string name_;
  std::string value_;
  int64_t expiry_date_;
  std::string comment_;
  std::string comment_url_;
  std::string domain_;
  std::string path_;
  std::string port_limitations_;
  bool secure_;
  bool http_only_;
  bool only_server_;
  int8_t version_number_;

  typedef base::Callback<void(DataStreamReader*)> TagHandler;
  typedef std::map<uint8_t, TagHandler> TagHandlersMap;
  TagHandlersMap tag_handlers_;

  void RegisterTagHandlers();
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_COOKIES_IMPORTED_COOKIE_H_
