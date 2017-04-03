// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/favorites/favorite.h"
#include "mobile/common/favorites/folder.h"

namespace mobile {

bool Favorite::HasAncestor(const Folder* folder) const {
  return HasAncestor(folder->id());
}

bool Favorite::HasAncestor(int64_t id) const {
  if (this->id() == id) return true;
  if (parent() == this->id()) return false;
  return GetParent()->HasAncestor(id);
}

}  // namespace mobile
