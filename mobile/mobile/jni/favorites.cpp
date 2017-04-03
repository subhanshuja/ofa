/* -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * Copyright (C) 2017 Opera Software AS.  All rights reserved.
 *
 * This file is an original work developed by Opera Software.
 */

#include <jni.h>
#include "jni_obfuscation.h"
#include "jniutils.h"

#include "favorites.h"
#include "pushedcontent.h"

#include "common/favorites/favorites_delegate.h"

namespace {

class MobileFavoritesDelegate : public mobile::FavoritesDelegate {
 public:
  MobileFavoritesDelegate(
      mobile::PartnerContentHandler *partner_content_handler)
      : partner_content_handler_(partner_content_handler) {}

  void SetRequestGraphicsSizes(__unused int icon_size,
                               int thumb_size) override {
    partner_content_handler_->SetThumbnailSize(thumb_size);
  }

  // The thumbnail handling is intercepted and handled in libopera.
  void RequestThumbnail(int64_t, const char *, const char *, bool,
                        ThumbnailReceiver *) override {}
  void CancelThumbnailRequest(int64_t) override {}

  void SetPartnerContentInterface(PartnerContentInterface *interface) override {
    partner_content_handler_->SetPartnerContentInterface(interface);
  }

  void OnPartnerContentActivated() override {
    partner_content_handler_->OnPartnerContentActivated();
  }

  void OnPartnerContentRemoved(int32_t partner_id) override {
    partner_content_handler_->OnPartnerContentChanged(partner_id);
  }

  void OnPartnerContentEdited(int32_t partner_id) override {
    partner_content_handler_->OnPartnerContentChanged(partner_id);
  }

  /**
   * Return suffix added to created thumbnails. May be NULL (equal to "")
   */
  const char *GetPathSuffix() const override {
    return NULL;
  }

 private:
  mobile::PartnerContentHandler *partner_content_handler_;
};

}  // namespace

namespace mobile {

FavoritesDelegate *setupFavoritesDelegate(PartnerContentHandler *handler) {
  return new MobileFavoritesDelegate(handler);
}

}  // namespace mobile
