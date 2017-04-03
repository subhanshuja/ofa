// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef DUPLICATE_TRACKER_INTERFACE_H
#define DUPLICATE_TRACKER_INTERFACE_H

#include <memory>

namespace base {
class ListValue;
}

namespace sync_bookmarks {
class BookmarkModelAssociator;
}

namespace opera {

class DuplicateTrackerInterface {
 public:
  virtual void SetBookmarkModelAssociator(
      sync_bookmarks::BookmarkModelAssociator* associator) = 0;
  virtual std::unique_ptr<base::ListValue> GetInternalStats() const = 0;
};

}  // namespace opera

#endif  // DUPLICATE_TRACKER_INTERFACE_H
