// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/history/history_suggestion_provider.h"

#include <algorithm>
#include <memory>

#include "base/i18n/string_search.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"

// These are included through mobile/common/sync/sync.gyp.
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/history/history_service_factory.h"

#include "components/history/core/browser/history_database.h"
#include "components/history/core/browser/history_db_task.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"

#include "content/public/browser/browser_thread.h"

#include "common/sync/sync.h"
#include "mobile/common/favorites/favorites.h"

namespace {

// Base score used when matching on url.
static const int HISTORY_URL_SCORE = 900;
// Base score used when matching on title.
static const int HISTORY_TITLE_SCORE = 800;
// Base score used when matching on root-domain.
static const int HISTORY_ROOTDOMAIN_SCORE = 1000;
// Base score used when matching on url which is a favorite
static const int HISTORY_FAVORITE_SCORE = 1100;
// Maximum score a history suggestion can be given.
static const int HISTORY_MAX_SCORE = 1199;

bool IsRootDomain(const GURL& url) {
  return url.path() == "/" || url.path().empty();
}

int CalculateBaseScore(const GURL& url, bool is_url, bool isInFavorites) {
  // Matches on urls which are also favorites should have a higher base score
  if (isInFavorites) {
    return HISTORY_FAVORITE_SCORE;
  }

  if (IsRootDomain(url)) {
    return HISTORY_ROOTDOMAIN_SCORE;
  }

  return is_url ? HISTORY_URL_SCORE : HISTORY_TITLE_SCORE;
}

int CalculateTitleScore(const history::URLRow& row, bool isInFavorites) {
  int base_score = CalculateBaseScore(row.url(), false, isInFavorites);
  return std::min(HISTORY_MAX_SCORE, base_score + row.visit_count());
}

int CalculateUrlScore(const history::URLRow& row, bool isInFavorites) {
  int base_score = CalculateBaseScore(row.url(), true, isInFavorites);
  return std::min(HISTORY_MAX_SCORE, base_score + row.visit_count());
}

class QueryFromHistoryDBTask : public history::HistoryDBTask {
 public:
  explicit QueryFromHistoryDBTask(opera::SuggestionCallback callback,
                                  const std::string phrase,
                                  const std::string type,
                                  const bool exclude_favorites);

  bool RunOnDBThread(history::HistoryBackend* backend,
                     history::HistoryDatabase* db) override;

  void DoneRunOnMainThread() override;

 private:
  opera::SuggestionCallback callback_;
  const std::string phrase_;
  const std::string type_;
  const bool exclude_favorites_;
  std::vector<opera::SuggestionItem> suggestions_;

  bool IsDuplicateUrl(const GURL& url);

  bool IsInFavorites(const GURL& url) const;

  bool SearchAndScore(
      const history::URLRow& row,
      base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents* fps,
      int* score);

  void RunCallback(opera::SuggestionCallback callback,
                   const std::vector<opera::SuggestionItem> suggestions);

  DISALLOW_COPY_AND_ASSIGN(QueryFromHistoryDBTask);
};

QueryFromHistoryDBTask::QueryFromHistoryDBTask(
    opera::SuggestionCallback callback,
    const std::string phrase,
    const std::string type,
    const bool exclude_favorites)
    : callback_(callback),
      phrase_(phrase),
      type_(type),
      exclude_favorites_(exclude_favorites) {}

bool QueryFromHistoryDBTask::RunOnDBThread(history::HistoryBackend* backend,
                                           history::HistoryDatabase* db) {
  // Initialize the enumerator used to iterate the all of the history rows
  history::URLDatabase::URLEnumerator history_enum;
  if (!db->InitURLEnumeratorForSignificant(&history_enum)) {
    return false;
  }

  // Initialize the a fixed pattern search using i18n string-search
  base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents fps(
      base::UTF8ToUTF16(phrase_));

  int score;
  for (history::URLRow row; history_enum.GetNextURL(&row);) {
    if (row.hidden()) {
      continue;
    }

    if (IsDuplicateUrl(row.url())) {
      continue;
    }

    if (!SearchAndScore(row, &fps, &score)) {
      continue;
    }

    opera::SuggestionItem item(score, base::UTF16ToUTF8(row.title()),
                               row.url().spec(), type_);
    suggestions_.push_back(item);
  }

  // SuggestionManager expects the suggestions to be sorted in
  // descending order by their relevance.
  std::sort(
      suggestions_.begin(), suggestions_.end(),
      [](const opera::SuggestionItem& it1, const opera::SuggestionItem& it2) {
        return it2.relevance < it1.relevance;
      });

  return true;
}

void QueryFromHistoryDBTask::DoneRunOnMainThread() {
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&QueryFromHistoryDBTask::RunCallback, base::Unretained(this),
                 callback_, suggestions_));
}

void QueryFromHistoryDBTask::RunCallback(
    opera::SuggestionCallback callback,
    const std::vector<opera::SuggestionItem> suggestions) {
  callback.Run(suggestions);
}

bool QueryFromHistoryDBTask::IsInFavorites(const GURL& url) const {
  mobile::Favorites* favorites = mobile::Favorites::instance();

  std::vector<int64_t> matches;
  favorites->GetFavoritesByURL(url, &matches);
  return (matches.size() > 0);
}

bool QueryFromHistoryDBTask::IsDuplicateUrl(const GURL& url) {
  for (std::vector<opera::SuggestionItem>::iterator it = suggestions_.begin();
       it < suggestions_.end(); it++) {
    GURL it_url((*it).url);
    if (it_url.GetContent() == url.GetContent()) {
      return true;
    }
  }

  return false;
}

bool QueryFromHistoryDBTask::SearchAndScore(
    const history::URLRow& row,
    base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents* fps,
    int* out_score) {
  *out_score = 0;
  size_t match_len, match_index;

  // Perform a title search
  if (fps->Search(row.title(), &match_index, &match_len)) {
    bool is_in_favorites = IsInFavorites(row.url());
    if (is_in_favorites && exclude_favorites_)
      return false;
    *out_score = CalculateTitleScore(row, is_in_favorites);
    return true;
  }

  // Perform a url search
  if (fps->Search(base::UTF8ToUTF16(row.url().spec()), &match_index,
                  &match_len)) {
    bool is_in_favorites = IsInFavorites(row.url());
    if (is_in_favorites && exclude_favorites_)
      return false;
    *out_score = CalculateUrlScore(row, is_in_favorites);
    return true;
  }

  return false;
}

}  // namespace

namespace opera {

HistorySuggestionProvider::HistorySuggestionProvider(bool exclude_favorites)
    : profile_(mobile::Sync::GetInstance()->GetProfile()),
      exclude_favorites_(exclude_favorites) {}

HistorySuggestionProvider::~HistorySuggestionProvider() {}

void HistorySuggestionProvider::Query(const SuggestionTokens& query,
                                      bool private_browsing,
                                      const std::string& type,
                                      const SuggestionCallback& callback) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  history::HistoryService* hs = GetHistoryService();

  if (hs) {
    hs->ScheduleDBTask(
        std::unique_ptr<history::HistoryDBTask>(new QueryFromHistoryDBTask(
            callback, query.phrase(), type, exclude_favorites_)),
        &cancelable_task_tracker_);
  }
}

history::HistoryService* HistorySuggestionProvider::GetHistoryService() {
  return HistoryServiceFactory::GetForProfile(
      profile_, ServiceAccessType::IMPLICIT_ACCESS);
}

}  // namespace opera
