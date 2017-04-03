// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_FAVORITES_THUMBNAIL_REQUEST_INTERCEPTOR_HELPER_H_
#define CHILL_BROWSER_FAVORITES_THUMBNAIL_REQUEST_INTERCEPTOR_HELPER_H_

// This file may not use STL

#include "mobile/common/favorites/favorites_delegate.h"

extern "C" __attribute__((visibility("default"))) void
ThumbnailRequestInterceptor_SetFavoritesDelegate(
    mobile::FavoritesDelegate* delegate);

typedef void (*ThumbnailRequestInterceptor_SetFavoritesDelegateSignature)(
    mobile::FavoritesDelegate* delegate);

#endif  // CHILL_BROWSER_FAVORITES_THUMBNAIL_REQUEST_INTERCEPTOR_HELPER_H_
