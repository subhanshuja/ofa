// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_FRAUD_PROTECTION_FRAUD_RATED_SERVER_H_
#define COMMON_FRAUD_PROTECTION_FRAUD_RATED_SERVER_H_

#include <list>
#include <map>
#include <string>

#include "base/callback.h"
#include "base/time/time.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

class XmlReader;

namespace net {
class URLRequestContextGetter;
}

namespace opera {
class FraudAdvisory;
class FraudRatedServerObserver;
class FraudUrlRating;

// FraudRatedServer keeps the fraud information (if any) about a server
// identified by hostname_.

class FraudRatedServer : public net::URLFetcherDelegate {
 public:
  typedef base::Callback<void(const FraudRatedServer*)> RatingCompleteCallback; // NOLINT
  FraudRatedServer(const std::string& hostname,
                   bool is_secure,
                   const RatingCompleteCallback& callback,
                   net::URLRequestContextGetter* request_context_getter);

  ~FraudRatedServer() override;

  const std::string& hostname() const { return hostname_; }

  bool IsExpired() const;
  bool IsRated(bool never_expire) const;
  void GetRatingForUrl(const GURL& url, FraudUrlRating* rating) const;

  void StartRating();
  void SetRatingBypassed();
  bool IsBypassed() const;

 private:
  typedef std::map<unsigned, FraudAdvisory> Advisories;

  enum State {
    SERVER_STATE_UNRATED,
    SERVER_STATE_RATING_IN_PROGRESS,
    SERVER_STATE_RATED
  };

  // Overridden from URLFetcher
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // Invoked after receiving and parsing rating document.
  void OnServerRated(bool result);

  // Parse XML sections
  void XMLHandleExpireTime(XmlReader& xml_parser);
  void XMLHandleSource(XmlReader& xml_parser);
  void XMLHandleFraudInfo(XmlReader& xml_parser);

  // Remove advisories that have no single fraud detector.
  void PruneInvalidAdvisories();

  // Server state
  State state_;

  // Server detais - hostname and is this a HTTPS:// secure server
  std::string hostname_;
  bool is_secure_;

  // Server rating expiration time
  base::Time expire_time_;

  // Server rating bypassed by user.
  bool rating_bypassed_by_user_;

  // List of advisories.
  Advisories advisories_;

  // Fetcher used to get the Fraud XML from Opera servers
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  RatingCompleteCallback rating_complete_callback_;

  // Application-specific context provider for URL requests.
  net::URLRequestContextGetter* request_context_getter_;
};

}  // namespace opera

#endif  // COMMON_FRAUD_PROTECTION_FRAUD_RATED_SERVER_H_
