// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_COOKIES_COOKIE_CALLBACK_H_
#define COMMON_MIGRATION_COOKIES_COOKIE_CALLBACK_H_

#include <string>

#include "base/callback.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}
namespace opera {
namespace common {
namespace migration {
/** This callback will get called once for every imported cookie, with the url
 * of the cookie and the RFC-2965 cookie line (without "Set-Cookie:"). It
 * is expected that the callback will add the cookie to Opera's cookie
 * storage. Will be called from FILE thread.
 */
typedef base::Callback<void(const GURL& url,
                            const std::string& cookie_string)> CookieCallback;

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_COOKIES_COOKIE_CALLBACK_H_
