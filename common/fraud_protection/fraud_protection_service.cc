// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/fraud_protection/fraud_protection_service.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/origin_util.h"
#include "content/public/common/url_constants.h"
#include "net/base/ip_address.h"
#include "net/base/url_util.h"
#include "url/url_constants.h"

#include "common/fraud_protection/fraud_rated_server.h"
#include "common/fraud_protection/fraud_url_rating.h"

namespace {

// Maximum size of the server list.
static const size_t kServerListMaxSize = 512;

}  // namespace

namespace opera {

FraudProtectionService::FraudProtectionService(
    net::URLRequestContextGetter* request_context_getter)
    : request_context_getter_(request_context_getter),
      weakptr_factory_(this) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  static const char* const kPrivateNetworks[] = {
    "10.0.0.0/8",
    "127.0.0.0/8",
    "172.16.0.0/12",
    "192.168.0.0/16",
    // IPv6 address ranges
    "fc00::/7",
    "fec0::/10",
    "::1/128",
  };

  for (size_t i = 0; i < arraysize(kPrivateNetworks); ++i) {
    net::IPAddress ip_address;
    size_t prefix_length;
    if (net::ParseCIDRBlock(kPrivateNetworks[i], &ip_address, &prefix_length)) {
      private_networks_.push_back(std::make_pair(ip_address, prefix_length));
    }
  }
}

FraudProtectionService::~FraudProtectionService() {
  for (RatedServers::iterator iter = rated_servers_.begin();
       iter != rated_servers_.end();
       ++iter) {
    delete iter->second;
  }
}

void FraudProtectionService::GetUrlRating(
    const GURL& url,
    const std::string& ip,
    const UrlCheckedCallback& callback) {

  FraudUrlRating rating;
  bool rating_known = false;

  // Don't check non-FTP/HTTP(S), nor intranet names
  if ((!url.SchemeIs(url::kFtpScheme) && !url.SchemeIs(url::kHttpScheme) &&
       !url.SchemeIs(url::kHttpsScheme)) ||
      net::IsHostnameNonUnique(url.host())) {
    rating.rating_ = FraudUrlRating::RATING_URL_NOT_RATED;
    rating_known = true;
  } else if (!ip.empty()) {  // Don't check hosts on the intranet
    net::IPAddress ip_address;
    if (ip_address.AssignFromIPLiteral(ip)) {
      for (AddressRanges::const_iterator it =
           private_networks_.begin();
           it != private_networks_.end(); ++it) {
        if (net::IPAddressMatchesPrefix(ip_address, it->first, it->second)) {
          rating.rating_ = FraudUrlRating::RATING_URL_NOT_RATED;
          rating_known = true;
        }
      }
    }
  }

  if (!rating_known) {
    FraudRatedServer* server = GetServerForURL(url);
    DCHECK(server);

    if (!server->IsRated(false)) {
      pending_requests_.insert(
          std::make_pair(server, std::make_pair(url, callback)));
      StartRating(server);
    } else {
      FillUrlRatingInfo(url, *server, &rating);
      rating_known = true;
    }
  }

  if (rating_known)
    callback.Run(rating);
}

void FraudProtectionService::BypassUrlRating(const FraudUrlRating& url_rating) {
  RatedServers::iterator iter = rated_servers_.find(url_rating.server_id_);
  if (iter != rated_servers_.end())
    iter->second->SetRatingBypassed();
}

bool FraudProtectionService::IsEnabled() const {
  return false;
}

FraudRatedServer* FraudProtectionService::GetServerForURL(const GURL& url) {
  DCHECK(url.has_host());

  RatedServers::iterator iter = rated_servers_.find(url.host());
  if (iter != rated_servers_.end())
    return iter->second;

  FraudRatedServer* new_server = new FraudRatedServer(
      url.host(),
      true,
      base::Bind(&FraudProtectionService::OnRatingComplete,
                 base::Unretained(this)),
      request_context_getter_);
  rated_servers_.insert(std::make_pair(new_server->hostname(), new_server));

  if (rated_servers_.size() > kServerListMaxSize) {
    ScheduleServerListClean();
  }

  return new_server;
}

void FraudProtectionService::StartRating(FraudRatedServer* server) {
  if (last_failed_request_ + grace_period_ < base::Time::NowFromSystemTime()) {
    server->StartRating();
  }
}

void FraudProtectionService::OnRatingComplete(const FraudRatedServer* server) {
  if (!server->IsRated(true)) {
    const base::TimeDelta kMinimumGracePreiod = base::TimeDelta::FromMinutes(4);
    const base::TimeDelta kMaximumGracePreiod =
        base::TimeDelta::FromMinutes(64);
    if (grace_period_ == base::TimeDelta()) {
      grace_period_ = kMinimumGracePreiod;
    } else {
      grace_period_ *= 2;
      if (grace_period_ > kMaximumGracePreiod) {
        grace_period_ = kMaximumGracePreiod;
      }
    }
  }

  std::pair<PendingRequests::iterator, PendingRequests::iterator> range =
    pending_requests_.equal_range(server);
  for (PendingRequests::iterator i = range.first; i != range.second; ++i) {
    FraudUrlRating rating;
    FillUrlRatingInfo(i->second.first, *server, &rating);
    i->second.second.Run(rating);
  }

  pending_requests_.erase(range.first, range.second);
}

void FraudProtectionService::FillUrlRatingInfo(const GURL& url,
                                               const FraudRatedServer& server,
                                               FraudUrlRating* rating) const {
  server.GetRatingForUrl(url, rating);
  rating->server_id_ = server.hostname();
}

void FraudProtectionService::ScheduleServerListClean() {
  base::Time now = base::Time::NowFromSystemTime();
  // Minimum delay between cache clean runs
  const base::TimeDelta kMinCacheCleanDelay = base::TimeDelta::FromMinutes(60);
  if (last_cache_clean_time_ + kMinCacheCleanDelay < now) {
    last_cache_clean_time_ = now;
    content::BrowserThread::PostTask(
        content::BrowserThread::UI,
        FROM_HERE,
        base::Bind(&FraudProtectionService::CleanExpiredServers,
                   weakptr_factory_.GetWeakPtr()));
  }
}

void FraudProtectionService::CleanExpiredServers() {
  DCHECK(rated_servers_.size() > kServerListMaxSize);

  RatedServers::iterator iter = rated_servers_.begin();
  while (iter != rated_servers_.end()) {
    if (iter->second->IsExpired()) {
      delete iter->second;
      rated_servers_.erase(iter++);
    } else {
      ++iter;
    }
  }
}

}  // namespace opera
