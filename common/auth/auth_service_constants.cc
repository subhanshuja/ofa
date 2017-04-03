// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2017 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#include "common/auth/auth_service_constants.h"

namespace opera {

const char kMobileClientHeader[] = "X-Mobile-Client";
const char kCSRFTokenHeader[] = "X-Opera-CSRF-token";
const char kErrorCodeHeader[] = "X-Opera-Auth-ErrCode";
const char kErrorMessageHeader[] = "X-Opera-Auth-ErrMsg";
const char kUserIDHeader[] = "X-Opera-Auth-UserID";
const char kUserEmailHeader[] = "X-Opera-Auth-UserEmail";
const char kUserNameHeader[] = "X-Opera-Auth-UserName";
const char kAuthTokenHeader[] = "X-OPera-Auth-AuthToken";
const char kEmailVerifiedHeader[] = "X-Opera-Auth-EmailVerified";

}  // namespace opera
