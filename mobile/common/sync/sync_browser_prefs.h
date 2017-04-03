// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#ifndef MOBILE_COMMON_SYNC_SYNC_BROWSER_PREFS_H_
#define MOBILE_COMMON_SYNC_SYNC_BROWSER_PREFS_H_

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace mobile {

// Register all prefs needed to get sync up and running.
void RegisterUserProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

}  // namespace mobile

#endif  // MOBILE_COMMON_SYNC_SYNC_BROWSER_PREFS_H_
