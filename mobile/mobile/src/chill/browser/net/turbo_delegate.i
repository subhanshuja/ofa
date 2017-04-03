// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

// Suppress "Returning a pointer or reference in a director method is not
// recommended."
%warnfilter(473) TurboDelegate;

%{
#include "chill/browser/net/turbo_delegate.h"
%}

VECTOR(StringList, std::string);

SWIG_SELFREF_NAMESPACED_CONSTRUCTOR(opera, TurboDelegate);

namespace opera {

%feature("director") TurboDelegate;

struct CountryInformation {
  bool is_mobile_connection;
  std::string mobile_country_code;
  std::string mobile_network_code;
};

class TurboDelegate {
 public:
  virtual ~TurboDelegate();

  virtual void OnTurboClientId(std::string client_id) = 0;
  virtual void OnTurboStatistics(unsigned compressed,
                                 unsigned uncompressed,
                                 unsigned videos_compressed_delta) = 0;
  virtual void OnTurboSuggestedServer(std::string suggested_server) = 0;
  virtual CountryInformation GetCountryInformation() = 0;
  virtual void FetchZeroRatingRules() = 0;
  virtual void SpoofMCCMNC(const std::string& mcc, const std::string& mnc) = 0;
  virtual std::string GetPrivateDataKey() = 0;
  virtual std::vector<std::string> GetHelloHeaders(bool is_secure) = 0;
};

}  // namespace opera
