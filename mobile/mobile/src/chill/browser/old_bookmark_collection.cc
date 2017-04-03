// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/old_bookmark_collection.h"

#include <unistd.h>

#include "base/path_service.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/browser_thread.h"

#include "common/old_bookmarks/bookmark_collection_impl.h"
#include "common/old_bookmarks/bookmark_db_storage.h"

namespace opera {

OldBookmarkCollectionContainer::OldBookmarkCollectionContainer(
    scoped_refptr<BookmarkCollectionImpl> collection)
    : collection_(collection),
      collection_task_runner_(base::ThreadTaskRunnerHandle::Get()) {
}

OldBookmarkCollectionContainer::~OldBookmarkCollectionContainer() {
  collection_task_runner_->PostTask(
      FROM_HERE, base::Bind(&BookmarkCollectionImpl::Shutdown, collection_));
}

BookmarkCollection* OldBookmarkCollectionContainer::GetCollection() {
  return collection_.get();
}

OldBookmarkCollectionContainer* OpenOldBookmarkCollection() {
  base::FilePath app_base_path;
  PathService::Get(base::DIR_ANDROID_APP_DATA, &app_base_path);
  base::FilePath db_file(app_base_path.AppendASCII(kBookmarkDatabaseFilename));

  if (access(db_file.value().c_str(), F_OK) != 0)
    return NULL;

  BookmarkDBStorage* storage = new BookmarkDBStorage(
      app_base_path,
      content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::DB));
  return new OldBookmarkCollectionContainer(
      make_scoped_refptr(new BookmarkCollectionImpl(
          storage,
          content::BrowserThread::GetTaskRunnerForThread(
              content::BrowserThread::UI))));
}

}  // namespace opera
