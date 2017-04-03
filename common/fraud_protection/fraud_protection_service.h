// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_FRAUD_PROTECTION_FRAUD_PROTECTION_SERVICE_H_
#define COMMON_FRAUD_PROTECTION_FRAUD_PROTECTION_SERVICE_H_

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/containers/hash_tables.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "net/base/ip_address.h"
#include "url/gurl.h"

#include "common/fraud_protection/fraud_rated_server.h"

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}  // namespace net

namespace opera {

class FraudUrlRating;

class FraudProtectionService {
 public:
  typedef base::Callback<void(const FraudUrlRating&)> UrlCheckedCallback;  // NOLINT

  FraudProtectionService(net::URLRequestContextGetter* request_context_getter);
  virtual ~FraudProtectionService();

  // Checks the given URL rating. If the host name is non-unique, or if
  // the optional URL's resolved ip address is given and private,
  // FraudProtectionService will exclude checks on such URLs.
  // UrlCheckedCallback is being invoked whenever URL rating becomes known.
  //
  // NOTICE: callback can be invoked syncronously if only URL rating
  // is already in the cache.
  void GetUrlRating(const GURL& url,
                    const std::string& ip,
                    const UrlCheckedCallback& callback);

  // Bypass the rating returned by GetUrlRating. This usually happens when
  // user proceeds to fraudlent page acknowledging the potential fraud risk.
  // This bypass remains set only for current session.
  void BypassUrlRating(const FraudUrlRating& url_rating);

  // Returns true if the fraud protection service is enabled and false if it's
  // not.
  virtual bool IsEnabled() const;

 private:
  typedef std::pair<net::IPAddress, size_t> AddressRange;
  typedef std::list<AddressRange> AddressRanges;
  typedef base::hash_map<std::string, FraudRatedServer*> RatedServers;
  typedef std::pair<GURL, UrlCheckedCallback> PendingRequest;
  typedef std::multimap<const FraudRatedServer*, PendingRequest> PendingRequests; // NOLINT

  // Gets the server matching given URL. Existing server from rated_servers_ is
  // returned if url matches the server hostname. New server is created and
  // added to rated_servers_ otherwise.
  FraudRatedServer* GetServerForURL(const GURL& url);
  void StartRating(FraudRatedServer* server);

  void OnRatingComplete(const FraudRatedServer* server);

  // Fill the FraudUrlRating object with data.
  void FillUrlRatingInfo(const GURL& url,
                         const FraudRatedServer& server,
                         FraudUrlRating* rating) const;

  // Clean the server list by removing expired servers.
  void ScheduleServerListClean();
  void CleanExpiredServers();

  // Application-specific context provider for URL requests.
  net::URLRequestContextGetter* request_context_getter_;

  AddressRanges private_networks_;

  // Servers ratings
  RatedServers rated_servers_;
  PendingRequests pending_requests_;

  // Request throttling
  base::Time last_failed_request_;
  base::TimeDelta grace_period_;

  // Cache cleaning
  base::Time last_cache_clean_time_;

  base::WeakPtrFactory<FraudProtectionService> weakptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FraudProtectionService);
};

}  // namespace opera

#endif  // COMMON_FRAUD_PROTECTION_FRAUD_PROTECTION_SERVICE_H_
