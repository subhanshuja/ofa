// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_PROFILE_SYNC_SERVICE_PARAMS_PROVIDER_H_
#define COMMON_SYNC_PROFILE_SYNC_SERVICE_PARAMS_PROVIDER_H_

#include "base/memory/singleton.h"

#include "common/sync/sync_types.h"

class Profile;

namespace opera {

// Singleton that providers project specific sync login data to be used on
// intialization of the ProfileSyncService.
class ProfileSyncServiceParamsProvider {
 public:
  class Provider {
   public:
    virtual ~Provider() {}

    virtual void GetProfileSyncServiceInitParamsForProfile(
        Profile* profile,
        OperaSyncInitParams* params) = 0;
  };

  // Returns an instance of the ProfileSyncServiceParamsProvider singleton.
  static ProfileSyncServiceParamsProvider* GetInstance();

  void set_provider(Provider* provider) { provider_ = provider; }
  const Provider* provider() const { return provider_; }

  void GetInitParamsForProfile(Profile* profile, OperaSyncInitParams* params);

 private:
  friend struct base::DefaultSingletonTraits<ProfileSyncServiceParamsProvider>;

  ProfileSyncServiceParamsProvider();
  virtual ~ProfileSyncServiceParamsProvider();

  Provider* provider_;
};

}  // namespace opera

#endif  // COMMON_SYNC_PROFILE_SYNC_SERVICE_PARAMS_PROVIDER_H_
