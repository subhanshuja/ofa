/* -*- Mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2017 Opera Software AS.  All rights reserved.
 *
 * This file is an original work developed by Opera Software.
 */

#ifndef PUSHED_CONTENT_H
#define PUSHED_CONTENT_H

#include "common/favorites/favorites_delegate.h"

void initPushedContent(JNIEnv *env);

namespace mobile {

class PartnerContentHandler {
 public:
  virtual ~PartnerContentHandler() {}

  virtual void SetPartnerContentInterface(
      FavoritesDelegate::PartnerContentInterface *interface) = 0;
  virtual void SetThumbnailSize(int thumbnail_size) = 0;
  virtual void OnPartnerContentActivated() = 0;
  virtual void OnPartnerContentChanged(int32_t partner_id) = 0;
};

PartnerContentHandler *partnerContentHandler();

}  // namespace mobile

#endif /* PUSHED_CONTENT_H */
