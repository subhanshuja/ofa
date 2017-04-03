// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_MIGRATION_OAUTH1_SESSION_DATA_H_
#define COMMON_OAUTH2_MIGRATION_OAUTH1_SESSION_DATA_H_

#include <string>

#include "base/time/time.h"

namespace opera {
namespace oauth2 {

struct OAuth1SessionData {
  OAuth1SessionData();
  ~OAuth1SessionData();

  bool IsComplete() const;
  void ReportCompletnessHistogram() const;

  // Username or email
  std::string login;
  std::string user_id;
  base::TimeDelta time_skew;
  std::string token;
  std::string token_secret;
};

}  // namespace oauth2
}  // namespace opera

#endif  // COMMON_OAUTH2_MIGRATION_OAUTH1_SESSION_DATA_H_
