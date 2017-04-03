// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_ANALYTICS_URL_VISIT_LISTENER_
#define CHILL_BROWSER_ANALYTICS_URL_VISIT_LISTENER_

#include <string>

#include "components/history/core/browser/history_service_observer.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/url_row.h"

class Profile;

namespace history {
class HistoryService;
}

namespace opera {

class URLVisitListener : public history::HistoryServiceObserver {
 public:
  URLVisitListener();
  virtual ~URLVisitListener();

  virtual void OnURLVisited(const std::string& url, bool is_redirect) = 0;
  void OnURLVisited(history::HistoryService* history_service,
                    ui::PageTransition transition,
                    const history::URLRow& row,
                    const history::RedirectList& redirects,
                    base::Time visit_time) override;

 private:
  Profile* profile_;
  history::HistoryService* GetHistoryService();
};

}  // namespace opera

#endif  // CHILL_BROWSER_ANALYTICS_URL_VISIT_LISTENER_
