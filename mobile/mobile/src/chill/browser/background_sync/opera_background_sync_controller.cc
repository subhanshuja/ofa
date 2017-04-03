// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software AS
// @copied-from chromium/src/chrome/browser/background_sync/background_sync_controller_impl.cc
// @final-synchronized

#include "chill/browser/background_sync/opera_background_sync_controller.h"

#include <map>
#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "chill/browser/background_sync/background_sync_launcher_android.h"
#include "content/public/browser/background_sync_parameters.h"
#include "content/public/browser/browser_context.h"

namespace opera {

OperaBackgroundSyncController::OperaBackgroundSyncController(
    content::BrowserContext* context)
    : context_(context) {}

OperaBackgroundSyncController::~OperaBackgroundSyncController() = default;

void OperaBackgroundSyncController::GetParameterOverrides(
    content::BackgroundSyncParameters* parameters) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (BackgroundSyncLauncherAndroid::ShouldDisableBackgroundSync()) {
    parameters->disable = true;
  }

  return;
}

void OperaBackgroundSyncController::NotifyBackgroundSyncRegistered(
    const GURL& origin) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK_EQ(origin, origin.GetOrigin());

  if (context_->IsOffTheRecord())
    return;
}

void OperaBackgroundSyncController::RunInBackground(bool enabled,
                                                   int64_t min_ms) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (context_->IsOffTheRecord())
    return;
  BackgroundSyncLauncherAndroid::LaunchBrowserIfStopped(enabled, min_ms);
}

}  // namespace opera
