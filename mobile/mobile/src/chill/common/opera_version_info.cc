// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/common/opera_content_client.h"
#include "mobile/common/sync/opera_version_info.h"

namespace opera_version_info {

// static
std::string GetProductName() {
  return "Opera";
}

// static
std::string GetProductNameAndVersionForUserAgent() {
  return opera::GetUserAgent();
}

}  // namespace opera_version_info
