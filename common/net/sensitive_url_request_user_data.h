// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_NET_SENSITIVE_URL_REQUEST_USER_DATA_H_
#define COMMON_NET_SENSITIVE_URL_REQUEST_USER_DATA_H_

#include "base/supports_user_data.h"

namespace opera {

/** A user data for URLRequest that marks it as sensitive
 *
 * The sensitive requests are hidden from extensions' WebRequest API so
 * that extensions cannot block or manipulate them.
 */
class SensitiveURLRequestUserData : public base::SupportsUserData::Data {
 public:
  static const void* kUserDataKey;

  static base::SupportsUserData::Data* Create();
};

}  // namespace opera

#endif  // COMMON_NET_SENSITIVE_URL_REQUEST_USER_DATA_H_
