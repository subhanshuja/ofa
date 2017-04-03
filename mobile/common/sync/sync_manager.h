// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_SYNC_SYNC_MANAGER_H_
#define MOBILE_COMMON_SYNC_SYNC_MANAGER_H_

#include <string>

namespace bookmarks {
class BookmarkModel;
}

namespace syncer {
class DeviceInfo;
}

class Profile;

namespace mobile {

struct SyncedTabData;

class SyncManager {
public:
  enum ServiceType {
    SERVICE_GCM,
    SERVICE_APN,
  };
  enum Build {
    BUILD_DEVELOPER,
    BUILD_INTERNAL,
    BUILD_PUBLIC,
  };
  enum Status {
    STATUS_SETUP,         // Waiting for initial username/password
    STATUS_RUNNING,       // Logged in and syncing
    STATUS_FAILURE,       // Failed to sync (probably auth error)
    STATUS_NETWORK_ERROR, // Network error
    STATUS_LOGIN_ERROR,   // Error getting auth token, before trying to login
                          // to sync server
  };

  class Observer {
   public:
    virtual void OnStatusChanged(Status newStatus) = 0;

   protected:
    Observer() {}
    virtual ~Observer() {}
  };

  virtual ~SyncManager();
  virtual bookmarks::BookmarkModel* GetBookmarkModel() = 0;

  virtual void AuthSetup() = 0;

  virtual void InvalidateAll() = 0;

  virtual Status GetStatus() = 0;

  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  virtual void Logout() = 0;

  virtual std::string GetDisplayName() = 0;

  virtual Profile* GetProfile() = 0;

  virtual void Flush() = 0;
  virtual void Shutdown() = 0;

  virtual void StartSessionRestore() = 0;
  virtual void FinishSessionRestore() = 0;
  virtual void InsertTab(int index, const SyncedTabData* data) = 0;
  virtual void RemoveTab(int index) = 0;
  virtual void UpdateTab(int index, const SyncedTabData* data) = 0;
  virtual void SetActiveTab(int index) = 0;

  // Returns an empty std::unique_ptr if no device matching the name was found
  virtual std::unique_ptr<syncer::DeviceInfo> GetDeviceInfoForName(
      const std::string& name) const = 0;
};

}

#endif  // MOBILE_COMMON_SYNC_SYNC_MANAGER_H_
