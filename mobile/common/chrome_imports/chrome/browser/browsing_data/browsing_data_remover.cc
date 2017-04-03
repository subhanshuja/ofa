// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/browsing_data_remover.h"

#include "base/logging.h"
#include "base/message_loop/message_loop.h"

BrowsingDataRemover::BrowsingDataRemover(
    content::BrowserContext* browser_context) {
}

BrowsingDataRemover::~BrowsingDataRemover() {
}

void BrowsingDataRemover::Remove(const TimeRange& time_range,
                                 int remove_mask,
                                 int origin_type_mask) {
}

void BrowsingDataRemover::AddObserver(Observer* observer) {
}

void BrowsingDataRemover::RemoveObserver(Observer* observer) {
}
