// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/favorites/favorites.h"

#include "base/logging.h"

namespace mobile {

Favorites::Favorites() {
}

void Favorites::Remove(Favorite* favorite) {
  DCHECK(favorite);
  Remove(favorite->id());
}

void Favorites::ClearThumbnail(Favorite* favorite) {
  DCHECK(favorite);
  ClearThumbnail(favorite->id());
}

bool Favorites::IsLocal(int64_t id) {
  Favorite* favorite = GetFavorite(id);
  DCHECK(favorite);
  if (!favorite) return false;
  return favorite->HasAncestor(local_root());
}

}  // namespace mobile
