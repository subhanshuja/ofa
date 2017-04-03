// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/signin_manager_base.h"

namespace {
const std::string g_empty;
}  // namespace

const std::string& SigninManagerBase::GetAuthenticatedAccountId() const {
  return g_empty;
}
