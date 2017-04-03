// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/old_bookmarks/bookmark_storage.h"

namespace opera {

BookmarkStorage::BookmarkRawDataHolder::BookmarkRawData::BookmarkRawData(
    const std::string& guid,
    int index,
    const base::string16& name,
    const std::string& url,
    int type,
    const std::string& parent_guid) {
  guid_ = guid;
  index_ = index;
  name_ = name;
  url_ = url;
  type_ = type;
  parent_guid_ = parent_guid;
}

BookmarkStorage::BookmarkRawDataHolder::BookmarkRawData::~BookmarkRawData() {
}

BookmarkStorage::BookmarkRawDataHolder::BookmarkRawDataHolder() {
}

BookmarkStorage::BookmarkRawDataHolder::~BookmarkRawDataHolder() {
}

}  // namespace opera
