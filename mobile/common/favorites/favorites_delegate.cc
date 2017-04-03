// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/favorites/favorites.h"
#include "mobile/common/favorites/favorites_delegate.h"

void SetFavoritesDelegate(mobile::FavoritesDelegate* delegate) {
  mobile::Favorites::instance()->SetDelegate(delegate);
}
