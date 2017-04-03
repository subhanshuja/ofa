// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/favorites/folder.h"

#include "base/logging.h"
#include "components/bookmarks/browser/bookmark_node.h"

namespace {

const GURL g_empty_url_;
const base::Time g_null_time_;

}  // namespace

namespace mobile {

bool Folder::IsFolder() const {
  return true;
}

bool Folder::IsSavedPage() const {
  return false;
}

bool Folder::CanTransformToFolder() const {
  return false;
}

const GURL& Folder::url() const {
  return g_empty_url_;
}

void Folder::SetURL(const GURL& url) {
  NOTREACHED();
}

void Folder::Add(Favorite* favorite) {
  Add(Size(), favorite);
}

int Folder::IndexOf(Favorite* favorite) {
  DCHECK(favorite);
  return IndexOf(favorite->id());
}

int32_t Folder::PartnerActivationCount() const {
  return 0;
}

void Folder::SetPartnerActivationCount(int32_t activation_count) {
  NOTREACHED();
}

int32_t Folder::PartnerChannel() const {
  return 0;
}

void Folder::SetPartnerChannel(int32_t channel) {
  NOTREACHED();
}

int32_t Folder::PartnerId() const {
  return 0;
}

void Folder::SetPartnerId(int32_t id) {
  NOTREACHED();
}

int32_t Folder::PartnerChecksum() const {
  return 0;
}

void Folder::SetPartnerChecksum(int32_t checksum) {
  NOTREACHED();
}

const base::Time& Folder::last_modified() const {
  const bookmarks::BookmarkNode* node = data();
  DCHECK(node);
  if (node) {
    return node->date_folder_modified();
  }
  return g_null_time_;
}

}  // namespace mobile
