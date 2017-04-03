// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/favorites/favorite_utils.h"

#include "url/gurl.h"

#include "common/favorites/favorite.h"

namespace opera {
namespace FavoriteUtils {

TitleMatch::TitleMatch()
    : node(NULL) {
}

TitleMatch::~TitleMatch() {}

void ForEachFavoriteInFolder(
    const opera::FavoriteFolder* folder,
    bool recurse,
    const base::Callback<void(opera::Favorite*, int index)>& task) {
  for (int i = 0; i < folder->GetChildCount(); i++) {
    opera::Favorite* favorite = folder->GetChildByIndex(i);
    task.Run(favorite, i);
    if (recurse && favorite->GetType() == opera::Favorite::kFavoriteFolder)
      ForEachFavoriteInFolder(static_cast<opera::FavoriteFolder*>(favorite),
                              false,
                              task);
  }
}

}  // namespace FavoriteUtils
}  // namespace opera
