/* -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * Copyright (C) 2017 Opera Software AS.  All rights reserved.
 *
 * This file is an original work developed by Opera Software.
 */

#ifndef JNI_FAVORITES_H
#define JNI_FAVORITES_H

namespace mobile {

class FavoritesDelegate;
class PartnerContentHandler;

FavoritesDelegate *setupFavoritesDelegate(PartnerContentHandler *handler);

}  // namespace mobile

#endif /* JNI_FAVORITES_H */
