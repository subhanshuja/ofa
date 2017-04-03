// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_HISTORY_HISTORY_SUGGESTION_PROVIDER_H_
#define COMMON_HISTORY_HISTORY_SUGGESTION_PROVIDER_H_

#include <string>
#include <vector>

#include "base/task/cancelable_task_tracker.h"
#include "common/suggestion/suggestion_callback.h"
#include "common/suggestion/suggestion_item.h"
#include "common/suggestion/suggestion_provider.h"
#include "common/suggestion/suggestion_tokens.h"

class Profile;

namespace history {
class HistoryService;
}

namespace opera {

class HistorySuggestionProvider : public opera::SuggestionProvider {
 public:
  explicit HistorySuggestionProvider(bool exclude_favorites);
  ~HistorySuggestionProvider() override;

  void Query(const SuggestionTokens& query,
             bool private_browsing,
             const std::string& type,
             const SuggestionCallback& callback) override;

  void Cancel() override {};

 private:
  history::HistoryService* GetHistoryService();

  Profile* profile_;
  base::CancelableTaskTracker cancelable_task_tracker_;
  bool exclude_favorites_;
};

}  // namespace opera

#endif  // COMMON_HISTORY_HISTORY_SUGGESTION_PROVIDER_H_
