// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "common/sync/sync.h"
#include "mobile/common/sync/synced_tab_data.h"
%}

namespace bookmarks {
class BookmarkModel;
}

namespace opera {
class SuggestionProvider;
}

namespace mobile {

%feature("director", assumeoverride=1) SyncObserver;
class SyncObserver {
 public:
  virtual ~SyncObserver() { }

  virtual void SyncIsReady() = 0;
 protected:
  SyncObserver() { }
};

%nodefaultctor Sync;
%nodefaultdtor Sync;
class Sync {
 public:
  static Sync* GetInstance();

  static void Shutdown();

  bool IsReady() const;

  void AddObserver(SyncObserver* observer);
  void RemoveObserver(SyncObserver* observer);

  void AuthSetup();

  opera::SuggestionProvider* GetBookmarkSuggestionProvider();

  bookmarks::BookmarkModel* GetBookmarkModel();
};

}

%rename(Equals) operator==;
%include "../common/sync/synced_tab_data.h"
