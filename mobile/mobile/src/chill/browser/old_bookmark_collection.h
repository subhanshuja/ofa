// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_OLD_BOOKMARK_COLLECTION_H_
#define CHILL_BROWSER_OLD_BOOKMARK_COLLECTION_H_

#include "base/memory/ref_counted.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace opera {

class BookmarkCollection;
class BookmarkCollectionImpl;

class OldBookmarkCollectionContainer {
 public:
  OldBookmarkCollectionContainer(
      scoped_refptr<BookmarkCollectionImpl> collection);
  virtual ~OldBookmarkCollectionContainer();

  BookmarkCollection* GetCollection();
 private:
  scoped_refptr<BookmarkCollectionImpl> collection_;
  scoped_refptr<base::SingleThreadTaskRunner> collection_task_runner_;
};

OldBookmarkCollectionContainer* OpenOldBookmarkCollection();

}  // namespace opera

#endif  // CHILL_BROWSER_OLD_BOOKMARK_COLLECTION_H_
