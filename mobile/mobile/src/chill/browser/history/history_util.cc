// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/history/history_util.h"

#include "base/strings/utf_string_conversions.h"
// These are included through mobile/common/sync/sync.gyp.
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/history/history_service_factory.h"

#include "components/history/core/browser/history_service.h"
#include "chill/browser/history/history_entry.h"
#include "common/sync/sync.h"

namespace opera {

// static
void HistoryUtil::ExportEntries(std::vector<opera::HistoryEntry> entries) {
  Profile* profile = mobile::Sync::GetInstance()->GetProfile();
  history::HistoryService* hs = HistoryServiceFactory::GetForProfile(
      profile, ServiceAccessType::IMPLICIT_ACCESS);
  if (hs != NULL) {
    for (HistoryEntry entry : entries) {
      hs->AddPage(entry.url, entry.visitTime, history::SOURCE_BROWSED);
      hs->SetPageTitle(entry.url, base::UTF8ToUTF16(entry.title));
    }
  }
}

}  // namespace opera
