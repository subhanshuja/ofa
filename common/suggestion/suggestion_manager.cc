// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/suggestion/suggestion_manager.h"

#include <functional>
#include <set>
#include <string>

#include "base/bind.h"
#include "base/location.h"

#include "common/suggestion/suggestion_tokens.h"

namespace opera {

namespace {

/**
 * Merge two sets of suggestion items.
 *
 * This routine will merge a source range of suggestion items into the given
 * destination list. The result is sorted in descending order w.r.t the
 * item relevance (as defined by the SuggestionItem operator<() method).
 *
 * If there are several items having the same (non-empty) URL, only the item
 * with the highest relevance is kept.
 *
 * @param dst_list The destination list that will be updated.
 * @param provider_id The id of the provider for the source items.
 * @param src_first Start of the source items.
 * @param src_end End of the source items (exclusive).
 *
 * @note dst_list and the range given by [src_first, src_end) must be sorted
 * in descending order.
 *
 * TODO(mage): We should compare canonical URLs instead of strings when doing
 * the duplicate removal thing.
 */
template <typename InputIterator>
void MergeSuggestions(
    std::list<std::pair<int, SuggestionItem> >& dst_list,
    int provider_id,
    InputIterator src_first,
    InputIterator src_end) {
  std::list<std::pair<int, SuggestionItem> >::iterator dst = dst_list.begin();
  InputIterator src = src_first;
  std::set<std::string> unique_urls;
  while (src != src_end || dst != dst_list.end()) {
    if (dst == dst_list.end() || (src != src_end && !(*src < dst->second))) {
      if (unique_urls.find(src->url) == unique_urls.end()) {
        if (!src->url.empty()) {
          unique_urls.insert(src->url);
        }
        dst_list.insert(dst, std::make_pair(provider_id, *src));
      }
      src++;
    } else if (unique_urls.find(dst->second.url) == unique_urls.end()) {
      if (!dst->second.url.empty()) {
        unique_urls.insert(dst->second.url);
      }
      dst++;
    } else {
      dst = dst_list.erase(dst);
    }
  }
}

const SuggestionItem& extractSuggestionItemFromPair(
    const std::pair<int, SuggestionItem>& value) {
  return value.second;
}

}  // anonymous namespace

SuggestionManager::SuggestionManager(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : task_runner_(task_runner),
      current_query_id_(0),
      pending_provider_queries_(0),
      allow_partial_result_(true) {
}

SuggestionManager::~SuggestionManager() {
  SuggestionProviders::iterator it = providers_.begin();
  for (; it != providers_.end(); it++) {
    delete it->first;
  }
}

void SuggestionManager::AddProvider(std::unique_ptr<SuggestionProvider> provider,
                                    const std::string& type) {
  // From now on, the SuggestionManager owns the SuggestionProvider.
  providers_.push_back(std::make_pair(provider.release(), type));
}

void SuggestionManager::Query(
    const SuggestionTokens& query, bool private_browsing,
    const SuggestionCallback& callback) {
  // Cancel any pending queries.
  CancelInternal();

  // Send the query to each suggestion provider.
  pending_provider_queries_ = providers_.size();
  SuggestionProviders::iterator it = providers_.begin();
  for (int provider_id = 0; it != providers_.end(); ++it, ++provider_id) {
    // Ensure that providers can't call SuggestionManager::Query again
    // before SuggestionManager::Query returns.
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&SuggestionManager::QueryInternal,
                   base::Unretained(this),
                   it->first,
                   query,
                   current_query_id_,
                   private_browsing,
                   it->second,
                   base::Bind(&SuggestionManager::HandleSuggestions,
                              base::Unretained(this),
                              callback,
                              current_query_id_,
                              provider_id)));
  }
}

void SuggestionManager::Cancel() {
  CancelInternal();
  // CancelInternal doesn't clear suggestions_ since results from
  // providers will be incrementally included in suggestions_.
  suggestions_.clear();
}

void SuggestionManager::CancelInternal() {
  // Safety precaution: Any pending queries that come in late (though they
  // shouldn't if all the providers respect the Cancel command) will be
  // silently ignored.
  current_query_id_++;

  // Cancel each suggestion provider.
  SuggestionProviders::iterator it = providers_.begin();
  for (; it != providers_.end(); it++) {
    // Ensure that providers can't call SuggestionManager::Cancel again
    // before SuggestionManager::Cancel returns.
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&SuggestionProvider::Cancel, base::Unretained(it->first)));
  }

  pending_provider_queries_ =  0;
}

void SuggestionManager::SetAllowPartialResult(bool allow_partial_result)
{
  allow_partial_result_ = allow_partial_result;
}

class SuggestionItemFilter
    : public std::unary_function<const std::pair<int, SuggestionItem>&, bool> {
 public:
  explicit SuggestionItemFilter(int provider_id) : provider_id_(provider_id) {}
  bool operator() (const std::pair<int, SuggestionItem>& item) const {
    return item.first == provider_id_;
  }

 private:
  int provider_id_;
};

void SuggestionManager::HandleSuggestions(
    const SuggestionCallback& callback, int query_id, int provider_id,
    const std::vector<SuggestionItem>& suggestions) {
  // Ignore invalid query responses.
  if (query_id != current_query_id_) {
    return;
  }

  // Any old suggestions from the given provider are now outdated, since we
  // have a newer response.
  suggestions_.remove_if(SuggestionItemFilter(provider_id));

  --pending_provider_queries_;

  // Merge new suggestions with suggestions_ (in place).
  MergeSuggestions(
      suggestions_, provider_id, suggestions.begin(), suggestions.end());

  if (allow_partial_result_ || pending_provider_queries_ == 0) {
    // Send the suggestions to the UI instance.
    std::vector<SuggestionItem> result(suggestions_.size());
    transform(
        suggestions_.begin(), suggestions_.end(), result.begin(),
        extractSuggestionItemFromPair);
    callback.Run(result);
  }
}

void SuggestionManager::QueryInternal(SuggestionProvider* provider,
                                      const SuggestionTokens& query,
                                      int query_id,
                                      bool private_browsing,
                                      const std::string& type,
                                      const SuggestionCallback& callback) {
  // Ignore already canceled queries
  if (query_id != current_query_id_) {
    return;
  }

  provider->Query(query, private_browsing, type, callback);
}

}  // namespace opera
