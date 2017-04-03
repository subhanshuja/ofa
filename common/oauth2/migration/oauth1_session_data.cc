// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/migration/oauth1_session_data.h"

#include "base/metrics/histogram_macros.h"

namespace opera {
namespace oauth2 {

OAuth1SessionData::OAuth1SessionData() {}
OAuth1SessionData::~OAuth1SessionData() {}

bool OAuth1SessionData::IsComplete() const {
  if (login.empty())
    return false;
  if (token.empty())
    return false;
  if (token_secret.empty())
    return false;

  return true;
}

void OAuth1SessionData::ReportCompletnessHistogram() const {
  const int kHasLogin = 1 << 1;
  const int kHasUserId = 1 << 2;
  const int kHasToken = 1 << 3;
  const int kHasTokenSecret = 1 << 4;

  const int kBoundary = 1 << 5;

  int completness = 0;
  if (!login.empty())
    completness |= kHasLogin;
  if (!user_id.empty())
    completness |= kHasUserId;
  if (!token.empty())
    completness |= kHasToken;
  if (!token_secret.empty())
    completness |= kHasTokenSecret;

  UMA_HISTOGRAM_ENUMERATION("Opera.OAuth2.Migration.OAuth1SessionCompletness",
                            completness, kBoundary);
}

}  // namespace oauth2
}  // namespace opera
