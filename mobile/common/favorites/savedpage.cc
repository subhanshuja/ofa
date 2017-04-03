// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/favorites/savedpage.h"

#include "base/logging.h"

namespace mobile {

bool SavedPage::IsFolder() const {
  return false;
}

bool SavedPage::IsSavedPage() const {
  return true;
}

bool SavedPage::IsPartnerContent() const {
  return false;
}

bool SavedPage::CanTransformToFolder() const {
  return false;
}

bool SavedPage::CanChangeParent() const {
  return false;
}

int32_t SavedPage::PartnerActivationCount() const {
  return 0;
}

void SavedPage::SetPartnerActivationCount(int32_t activation_count) {
  NOTREACHED();
}

int32_t SavedPage::PartnerChannel() const {
  return 0;
}

void SavedPage::SetPartnerChannel(int32_t channel) {
  NOTREACHED();
}

int32_t SavedPage::PartnerId() const {
  return 0;
}

void SavedPage::SetPartnerId(int32_t id) {
  NOTREACHED();
}

int32_t SavedPage::PartnerChecksum() const {
  return 0;
}

void SavedPage::SetPartnerChecksum(int32_t checksum) {
  NOTREACHED();
}

}  // namespace mobile
