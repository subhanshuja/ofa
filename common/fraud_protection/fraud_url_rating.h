// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_FRAUD_PROTECTION_FRAUD_URL_RATING_H_
#define COMMON_FRAUD_PROTECTION_FRAUD_URL_RATING_H_

#include <string>

namespace opera {

class FraudUrlRating {
 public:
  enum Rating {
    // URL cannot be clasified. It is either because URL uses a browser private
    // URL scheme (like opera://) or resolved IP address belongs to private
    // IP adress range (172..., 192..., 10... etc).
    RATING_URL_NOT_RATED,

    // URL not clssified as fraudlent. Does not mean this URL is safe,
    // but none from existing advisories could classify it as fraudlent.
    RATING_URL_FRAUD_NOT_DETECTED,

    // Fraudlent URLs
    RATING_URL_PHISHING,
    RATING_URL_MALWARE
  };

  FraudUrlRating();
  FraudUrlRating(const FraudUrlRating&);
  ~FraudUrlRating();

  // Get the URL rating
  Rating rating() const { return rating_; }

  // Was the rating already bypassed?
  bool IsBypassed() const { return server_bypassed_; }

  // When URL has been rated as fraudlent, these members can be used to
  // obtain details about advisory that marked URL as fraudlent.
  const std::string& advisory_text() const { return advisory_text_; }
  const std::string& advisory_url() const { return advisory_url_; }
  const std::string& advisory_homepage() const { return advisory_homepage_; }
  const std::string& advisory_logo_url() const { return advisory_logo_url_; }

 private:
  friend class FraudProtectionService;
  friend class FraudRatedServer;

  Rating rating_;
  std::string advisory_homepage_;
  std::string advisory_url_;
  std::string advisory_text_;
  std::string advisory_logo_url_;

  bool server_bypassed_;

  // Server id. Used internally by FraudProtectionService to identify server
  // for purposes of bypassing.
  std::string server_id_;
};

}  // namespace opera

#endif  // COMMON_FRAUD_PROTECTION_FRAUD_URL_RATING_H_
