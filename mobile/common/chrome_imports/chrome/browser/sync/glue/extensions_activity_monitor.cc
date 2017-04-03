// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/glue/extensions_activity_monitor.h"

#include "components/sync/base/extensions_activity.h"

namespace browser_sync {

ExtensionsActivityMonitor::ExtensionsActivityMonitor()
    : extensions_activity_(new syncer::ExtensionsActivity()) {
}

ExtensionsActivityMonitor::~ExtensionsActivityMonitor() {
}

void ExtensionsActivityMonitor::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
}

const scoped_refptr<syncer::ExtensionsActivity>&
ExtensionsActivityMonitor::GetExtensionsActivity() {
  return extensions_activity_;
}

}  // namespace browser_sync
