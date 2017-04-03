// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/account/account_service.h"

namespace opera {

AccountService::AccountService() : weak_ptr_factory_(this) {}

AccountService::~AccountService() {}

base::WeakPtr<AccountService> AccountService::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace opera
