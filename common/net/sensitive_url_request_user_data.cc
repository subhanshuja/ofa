// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/net/sensitive_url_request_user_data.h"

namespace opera {

const void* SensitiveURLRequestUserData::kUserDataKey =
    static_cast<const void*>(&SensitiveURLRequestUserData::kUserDataKey);

base::SupportsUserData::Data* SensitiveURLRequestUserData::Create() {
  return new SensitiveURLRequestUserData();
}

}  // namespace opera
