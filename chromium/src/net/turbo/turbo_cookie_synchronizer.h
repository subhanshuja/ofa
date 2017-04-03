// Copyright (c) 2013 Opera Software ASA. All rights reserved.

#ifndef NET_TURBO_TURBO_COOKIE_SYNCHRONIZER_H_
#define NET_TURBO_TURBO_COOKIE_SYNCHRONIZER_H_

#include <map>
#include <set>
#include <string>
#include <utility>

#include "base/memory/weak_ptr.h"
#include "net/cookies/canonical_cookie.h"

namespace net {

class CookieStore;
class SpdySession;
class TurboSessionPool;

class NET_EXPORT TurboCookieSynchronizer {
 public:
  explicit TurboCookieSynchronizer(
      base::WeakPtr<TurboSessionPool> turbo_session_pool);
  ~TurboCookieSynchronizer();

  // Force resynchronizing all cookies for the given URL. This should be called
  // when a pushed stream fails cookie validation.
  void ForceSyncCookiesForURL(CookieStore* cookie_store, const GURL& url);
  // Notify that a cookie has been changed by the renderer and will need to be
  // synced before the next Turbo request.
  void OnCookieChanged(CookieStore* cookie_store,
                       const GURL& url,
                       std::string cookie_line);
  // Send any cookies that have been recorded as modified by OnCookieChanged.
  void SendScheduledCookies(CookieStore* cookie_store);

 private:
  void SendCookies(const GURL& url, std::set<std::string> names,
                   const CookieList& cookies);

  base::WeakPtr<TurboSessionPool> turbo_session_pool_;

  // Names of cookies scheduled for synchronization per URL.
  std::map<GURL, std::set<std::string>> scheduled_;

  base::WeakPtrFactory<TurboCookieSynchronizer> weak_ptr_factory_;
};

}  // namespace net

#endif  // NET_TURBO_TURBO_COOKIE_SYNCHRONIZER_H_
