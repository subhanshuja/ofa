// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2017 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#ifndef COMMON_AUTH_AUTH_SERVICE_ERROR_H_
#define COMMON_AUTH_AUTH_SERVICE_ERROR_H_

namespace opera {

enum AuthServiceError {
  NONE = 0,
  CONNECTION_FAILED = 1,
  INVALID_CREDENTIALS = 2,
  INTERNAL_ERROR = 3,
  UNEXPECTED_RESPONSE = 4,
};

}  // namespace opera

#endif  // COMMON_AUTH_AUTH_SERVICE_ERROR_H_
