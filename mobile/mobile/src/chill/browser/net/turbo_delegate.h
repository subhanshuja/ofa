// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_NET_TURBO_DELEGATE_H_
#define CHILL_BROWSER_NET_TURBO_DELEGATE_H_

#include <string>
#include <vector>

#include "base/android/scoped_java_ref.h"
#include "base/memory/ref_counted.h"
#include "base/observer_list_threadsafe.h"

namespace net {
class HttpResponseHeaders;
class URLRequest;
}  // namespace net

namespace opera {

struct CountryInformation {
  bool is_mobile_connection;
  std::string mobile_country_code;
  std::string mobile_network_code;
};

class TurboDelegate : public base::android::ScopedJavaGlobalRef<jobject> {
 public:
  class Observer {
   public:
    virtual void OnShouldUpdateRules(std::string update_message) = 0;
    virtual void OnOperaTrDebug(std::string tr_debug) = 0;
    virtual void OnTurboBypass(std::string bypass_warning) = 0;
  };

  TurboDelegate();
  virtual ~TurboDelegate();

  virtual void OnTurboClientId(std::string client_id) = 0;
  virtual void OnTurboStatistics(unsigned compressed_delta,
                                 unsigned uncompressed_delta,
                                 unsigned int videos_compressed_delta) = 0;
  virtual void OnTurboSuggestedServer(std::string suggested_server) = 0;

  /**
   * Return country information for current mobile connection.
   *
   * If the currently used connection is over a mobile network interface (GSM,
   * 3G, etc) then is_mobile_connection will be true and the contry code fields
   * will contain valid data. If the connection is not over a mobile network
   * interface (e.g. WiFi) then is_mobile_connection will be false (and the
   * content of country code fields is undefined).
   */
  virtual CountryInformation GetCountryInformation() = 0;

  virtual void FetchZeroRatingRules() = 0;

  virtual void SpoofMCCMNC(const std::string& mcc, const std::string& mnc) = 0;

  virtual std::string GetPrivateDataKey() = 0;
  virtual std::vector<std::string> GetHelloHeaders(bool is_secure) = 0;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void OnHeadersReceived(const net::URLRequest* request,
                         const net::HttpResponseHeaders* headers);

 private:
  bool ShouldNotifyObservers(const net::URLRequest* request,
                             const net::HttpResponseHeaders* headers);

  void NotifyObservers(const net::URLRequest* request,
                       const net::HttpResponseHeaders* headers);

  scoped_refptr<base::ObserverListThreadSafe<Observer> > observers_;

  DISALLOW_COPY_AND_ASSIGN(TurboDelegate);
};

}  // namespace opera

#endif  // CHILL_BROWSER_NET_TURBO_DELEGATE_H_
