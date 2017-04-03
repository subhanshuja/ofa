// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software AS
// @copied-from chromium/src/chrome/browser/background_sync/background_sync_controller_impl.h
// @final-synchronized

#ifndef CHILL_BROWSER_BACKGROUND_SYNC_OPERA_BACKGROUND_SYNC_CONTROLLER_H_
#define CHILL_BROWSER_BACKGROUND_SYNC_OPERA_BACKGROUND_SYNC_CONTROLLER_H_

#include "content/public/browser/background_sync_controller.h"

#include <stdint.h>

#include "base/macros.h"
#include "content/public/browser/browser_thread.h"

namespace content {
class BrowserContext;
struct BackgroundSyncParameters;
}

namespace opera {

class OperaBackgroundSyncController : public content::BackgroundSyncController {
 public:
  explicit OperaBackgroundSyncController(content::BrowserContext* context);
  ~OperaBackgroundSyncController() override;

  // content::BackgroundSyncController overrides.
  void GetParameterOverrides(
      content::BackgroundSyncParameters* parameters) const override;
  void NotifyBackgroundSyncRegistered(const GURL& origin) override;
  void RunInBackground(bool enabled, int64_t min_ms) override;

 private:
  content::BrowserContext* context_;

  DISALLOW_COPY_AND_ASSIGN(OperaBackgroundSyncController);
};

}  // namespace opera

#endif  // CHILL_BROWSER_BACKGROUND_SYNC_OPERA_BACKGROUND_SYNC_CONTROLLER_H_
