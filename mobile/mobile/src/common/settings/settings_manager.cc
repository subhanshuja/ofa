// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/settings/settings_manager.h"

#include "base/base64.h"
#include "base/logging.h"

#include "common/settings/settings_manager_delegate.h"

namespace opera {

// static
SettingsManagerDelegate* SettingsManager::delegate_ = NULL;

void SettingsManager::SetDelegate(opera::SettingsManagerDelegate* delegate) {
  delegate_ = delegate;
}

bool SettingsManager::GetJavaScript() {
  DCHECK(delegate_);
  return delegate_->GetJavaScript();
}

bool SettingsManager::GetUseDesktopUserAgent() {
  DCHECK(delegate_);
  return delegate_->GetUseDesktopUserAgent();
}

SettingsManager::BlockPopups SettingsManager::GetBlockPopups() {
  DCHECK(delegate_);
  return static_cast<BlockPopups>(delegate_->GetBlockPopupsInt());
}

SettingsManager::Cookies SettingsManager::GetCookies() {
  DCHECK(delegate_);
  return static_cast<Cookies>(delegate_->GetCookiesInt());
}

bool SettingsManager::GetTextWrap() {
  DCHECK(delegate_);
  return delegate_->GetTextWrap();
}

bool SettingsManager::GetForceEnableZoom() {
  DCHECK(delegate_);
  return delegate_->GetForceEnableZoom();
}

bool SettingsManager::GetFraudProtection() {
  DCHECK(delegate_);
  return delegate_->GetFraudProtection();
}

bool SettingsManager::GetReadingMode() {
  DCHECK(delegate_);
  return delegate_->GetReadingMode();
}

bool SettingsManager::GetCompression() {
  DCHECK(delegate_);
  return delegate_->GetCompression();
}

bool SettingsManager::GetAdBlocking() {
  DCHECK(delegate_);
  return delegate_->GetAdBlocking();
}

bool SettingsManager::GetVideoCompression() {
  DCHECK(delegate_);
  return delegate_->GetVideoCompression();
}

net::TurboImageQuality SettingsManager::GetTurboImageQualityMode() {
  DCHECK(delegate_);
  return delegate_->GetTurboImageQualityMode();
}

std::string SettingsManager::GetTurboClientId() {
  DCHECK(delegate_);
  std::string client_id;
  if (base::Base64Decode(delegate_->GetTurboClientId(), &client_id))
    return client_id;
  else
    return "invalid base64 data";
}

std::string SettingsManager::GetTurboDeviceId() {
  DCHECK(delegate_);
  return delegate_->GetTurboDeviceId();
}

std::string SettingsManager::GetTurboSuggestedServer() {
  DCHECK(delegate_);
  return delegate_->GetTurboSuggestedServer();
}

}  // namespace opera
