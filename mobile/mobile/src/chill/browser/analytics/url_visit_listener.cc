// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/analytics/url_visit_listener.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/history/history_service_factory.h"

#include "components/history/core/browser/history_service.h"

#include "common/sync/sync.h"

namespace opera {

URLVisitListener::URLVisitListener()
    : profile_(mobile::Sync::GetInstance()->GetProfile()) {
  history::HistoryService* hs = GetHistoryService();
  if (hs) {
    hs->AddObserver(this);
  }
}

URLVisitListener::~URLVisitListener() {
  history::HistoryService* hs = GetHistoryService();
  if (hs) {
    hs->RemoveObserver(this);
  }
}

void URLVisitListener::OnURLVisited(const std::string& url, bool is_redirect) {
  NOTREACHED();
}

void URLVisitListener::OnURLVisited(history::HistoryService* history_service,
                                   ui::PageTransition transition,
                                   const history::URLRow& row,
                                   const history::RedirectList& redirects,
                                   base::Time visit_time) {
  OnURLVisited(row.url().spec(), ui::PageTransitionIsRedirect(transition));
}

history::HistoryService* URLVisitListener::GetHistoryService() {
  return HistoryServiceFactory::GetForProfile(
      profile_, ServiceAccessType::IMPLICIT_ACCESS);
}

}  // namespace opera
