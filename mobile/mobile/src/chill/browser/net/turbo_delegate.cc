// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/net/turbo_delegate.h"

#include <vector>

#include "base/logging.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"

namespace opera {

namespace {

bool ShouldUpdateZeroRatingRules(const net::HttpResponseHeaders* headers) {
  std::string opera_info_value;

  return (headers->response_code() == 503 &&
          headers->EnumerateHeader(NULL, "x-opera-info", &opera_info_value) &&
          opera_info_value == "e=3");
}

const std::string kDebugInfoHeader = "x-opera-trdebug";

}  // namespace

TurboDelegate::TurboDelegate()
    : observers_(new base::ObserverListThreadSafe<Observer>()) {
}

TurboDelegate::~TurboDelegate() {
}

void TurboDelegate::AddObserver(Observer* observer) {
  observers_->AddObserver(observer);
}

void TurboDelegate::RemoveObserver(Observer* observer) {
  observers_->RemoveObserver(observer);
}

void TurboDelegate::OnHeadersReceived(const net::URLRequest* request,
                                      const net::HttpResponseHeaders* headers) {
  if (ShouldNotifyObservers(request, headers)) {
    NotifyObservers(request, headers);
  }

  if (ShouldUpdateZeroRatingRules(headers)) {
    FetchZeroRatingRules();
  }
}

// We're interested in responses where the debug header is present or situations
// where we should have received the header but didn't (non-https traffic)
bool TurboDelegate::ShouldNotifyObservers(
    const net::URLRequest* request,
    const net::HttpResponseHeaders* headers) {
  return headers->HasHeader(kDebugInfoHeader) ||
         ShouldUpdateZeroRatingRules(headers) ||
         !request->original_url().SchemeIs("https");
}

void TurboDelegate::NotifyObservers(const net::URLRequest* request,
                                    const net::HttpResponseHeaders* headers) {
  const std::string url = request->original_url().spec();

  if (ShouldUpdateZeroRatingRules(headers)) {
    std::string message("url = " + url + " resulted in x-opera-info: e=3");
    observers_->Notify(FROM_HERE, &TurboDelegate::Observer::OnShouldUpdateRules,
                       message);
  } else if (headers->HasHeader(kDebugInfoHeader)) {
    std::string tr_debug = "url = " + url;

    std::string tr_debug_part;
    size_t iter = 0;

    while (headers->EnumerateHeader(&iter, kDebugInfoHeader, &tr_debug_part)) {
      tr_debug += ", " + tr_debug_part;
    }

    observers_->Notify(FROM_HERE, &TurboDelegate::Observer::OnOperaTrDebug,
                       tr_debug);
  } else {
    std::string warning = "WARNING: " + url + " bypassed the Turbo servers";
    observers_->Notify(FROM_HERE, &TurboDelegate::Observer::OnTurboBypass,
                       warning);
  }
}

}  // namespace opera
