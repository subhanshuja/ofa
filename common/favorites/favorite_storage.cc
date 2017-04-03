// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/favorites/favorite_storage.h"

namespace opera {

FavoriteStorage::FavoriteRawDataHolder::FavoriteRawData::FavoriteRawData(
      const std::string& guid,
      int index,
      const base::string16& name,
      const std::string& url,
      int type,
      const std::string& parent_guid,
      const base::Pickle& serialized_data) {
  guid_ = guid;
  index_ = index;
  name_ = name;
  url_ = url;
  type_ = type;
  parent_guid_ = parent_guid;
  serialized_data_ = serialized_data;
}

FavoriteStorage::FavoriteRawDataHolder::FavoriteRawData::~FavoriteRawData() {
}

FavoriteStorage::FavoriteRawDataHolder::FavoriteRawDataHolder() {
}

FavoriteStorage::FavoriteRawDataHolder::~FavoriteRawDataHolder() {
}

}  // namespace opera
