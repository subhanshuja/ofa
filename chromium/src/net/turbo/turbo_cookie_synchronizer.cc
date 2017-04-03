// Copyright (c) 2013 Opera Software ASA. All rights reserved.

#include "net/turbo/turbo_cookie_synchronizer.h"

#include <map>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_util.h"
#include "net/cookies/cookie_store.h"
#include "net/cookies/parsed_cookie.h"
#include "net/http/http_stream_factory.h"
#include "net/spdy/spdy_session.h"
#include "net/turbo/turbo_session_pool.h"
#include "url/gurl.h"

namespace net {

namespace {

bool IsTurboEnabled() {
  return HttpStreamFactory::turbo2_enabled();
}

}  // namespace

TurboCookieSynchronizer::TurboCookieSynchronizer(
    base::WeakPtr<TurboSessionPool> turbo_session_pool)
    : turbo_session_pool_(turbo_session_pool), weak_ptr_factory_(this) {}

void TurboCookieSynchronizer::ForceSyncCookiesForURL(CookieStore* cookie_store,
                                                     const GURL& url) {
  DCHECK(turbo_session_pool_ && turbo_session_pool_->HasProxySession());

  if (cookie_store) {
    cookie_store->GetAllCookiesForURLAsync(
        url, base::Bind(&TurboCookieSynchronizer::SendCookies,
                        weak_ptr_factory_.GetWeakPtr(), url,
                        std::set<std::string>()));
  }
}

void TurboCookieSynchronizer::OnCookieChanged(CookieStore* cookie_store,
                                              const GURL& url,
                                              std::string cookie_line) {
  // Add changed cookies to a set of things we need to synchronize on the next
  // network request.
  if (!turbo_session_pool_ || !turbo_session_pool_->HasProxySession() ||
      !IsTurboEnabled())
    return;

  ParsedCookie parsed_cookie(cookie_line);

  if (parsed_cookie.IsValid()) {
    scheduled_[url].insert(parsed_cookie.Name());
  }
}

TurboCookieSynchronizer::~TurboCookieSynchronizer() {
  DCHECK(!turbo_session_pool_);
}

namespace {

std::string FormatTime(const base::Time& time) {
  const char* days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
  const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
                           "Aug", "Sep", "Oct", "Nov", "Dec" };
  char buffer[64];

  base::Time::Exploded exploded;
  time.UTCExplode(&exploded);

  int day_of_week = exploded.day_of_week % 7;
  if (day_of_week < 0)
    day_of_week = 0;

  int month = (exploded.month - 1) % 12;
  if (month < 0)
    month = 0;

  if (base::snprintf(buffer, sizeof buffer, "%s, %d %s %d %02d:%02d:%02d GMT",
                     days[day_of_week],
                     exploded.day_of_month,
                     months[month],
                     exploded.year,
                     exploded.hour,
                     exploded.minute,
                     exploded.second) < static_cast<int>(sizeof buffer))
    return buffer;

  return std::string();
}

std::string GetCookieLine(const CanonicalCookie& cookie) {
  std::string line = cookie.Name() + "=" + cookie.Value();

  if (!cookie.Domain().empty())
    line += "; Domain=" + cookie.Domain();
  if (!cookie.Path().empty())
    line += "; Path=" + cookie.Path();
  if (!cookie.ExpiryDate().is_null())
    line += "; Expires=" + FormatTime(cookie.ExpiryDate());
  if (cookie.IsSecure())
    line += "; Secure";
  if (cookie.IsHttpOnly())
    line += "; HttpOnly";

  if (cookie.Priority() != COOKIE_PRIORITY_DEFAULT)
    line += "; Priority=" + CookiePriorityToString(cookie.Priority());

  return line;
}

}  // namespace

void TurboCookieSynchronizer::SendScheduledCookies(CookieStore* cookie_store) {
  auto iter = scheduled_.begin();
  for (; iter != scheduled_.end(); iter = scheduled_.erase(iter)) {
    if (iter->second.empty()) {
      continue;
    }
    cookie_store->GetAllCookiesForURLAsync(
        iter->first,
        base::Bind(&TurboCookieSynchronizer::SendCookies,
                   weak_ptr_factory_.GetWeakPtr(), iter->first, iter->second));
  }
}

void TurboCookieSynchronizer::SendCookies(
    const GURL& url, std::set<std::string> names, const CookieList& cookies) {
  if (!turbo_session_pool_ || !turbo_session_pool_->HasProxySession())
    return;

  std::vector<std::string> cookie_lines;

  for (CookieList::const_iterator iter(cookies.begin());
       iter != cookies.end(); ++iter) {
    const CanonicalCookie& cookie(*iter);

    if (!names.empty() && !names.count(cookie.Name()))
      continue;

    const GURL& cookie_source = url.GetOrigin();
    std::string cookie_line = GetCookieLine(cookie);

    std::pair<GURL, std::string> cookie_data(cookie_source, cookie_line);

    cookie_lines.push_back(cookie_line);

    if (cookie_lines.size() == 255) {
      turbo_session_pool_->GetProxySession()->SendSetCookies(url.host(),
                                                             cookie_lines);
      cookie_lines.clear();
    }
  }

  if (cookie_lines.size()) {
    turbo_session_pool_->GetProxySession()->SendSetCookies(url.host(),
                                                           cookie_lines);
  }
}

}  // namespace net
