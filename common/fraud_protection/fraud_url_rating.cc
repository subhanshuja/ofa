// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/fraud_protection/fraud_url_rating.h"

#include "common/fraud_protection/fraud_advisory.h"

namespace opera {

FraudUrlRating::FraudUrlRating()
    : rating_(RATING_URL_NOT_RATED),
      server_bypassed_(false) {}

FraudUrlRating::FraudUrlRating(const FraudUrlRating&) = default;

FraudUrlRating::~FraudUrlRating() {
}

}  // namespace opera
