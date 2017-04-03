// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_H_
#define COMMON_SYNC_SYNC_H_

#include <jni.h>
#include <string>

#include "base/files/file_path.h"
#include "base/observer_list.h"

#include "mobile/common/sync/sync_manager.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace bookmarks {
class BookmarkModel;
}

namespace content {
class BrowserContext;
}

namespace net {
class URLRequestContextGetter;
}

namespace opera {
class SuggestionProvider;
}

class Profile;

namespace mobile {

class AuthData;
class FavoriteBookmarkDelegate;
class Invalidation;

class SyncObserver {
 public:
  virtual ~SyncObserver() { }

  virtual void SyncIsReady() = 0;
 protected:
  SyncObserver() { }
};

class Sync : public SyncManager::Observer {
 public:
  static Sync* GetInstance();

  static void Shutdown();

  static bool RegisterJni(JNIEnv* env);

  bool IsReady() const { return sync_manager_.get() != nullptr; }

  void AddObserver(SyncObserver* observer);
  void RemoveObserver(SyncObserver* observer);

  void AuthSetup();

  Profile* GetProfile();

  void Initialize(
      base::FilePath base_path,
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
      content::BrowserContext* browser_context);

  opera::SuggestionProvider* GetBookmarkSuggestionProvider();

  bookmarks::BookmarkModel* GetBookmarkModel();

  SyncManager* GetSyncManager();

  Invalidation* GetInvalidation();

  // SyncManager::Observer implementation
  void OnStatusChanged(SyncManager::Status newStatus) override;

 private:
  Sync();
  virtual ~Sync();

  std::unique_ptr<SyncManager> sync_manager_;
  std::unique_ptr<Invalidation> invalidation_;
  std::unique_ptr<AuthData> auth_data_;
  std::unique_ptr<opera::SuggestionProvider> bookmark_suggestion_provider_;
  base::ObserverList<SyncObserver> observers_;
};

}  // namespace mobile

#endif  // COMMON_SYNC_SYNC_H_
