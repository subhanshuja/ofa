// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SUGGESTION_SUGGESTION_MANAGER_H_
#define COMMON_SUGGESTION_SUGGESTION_MANAGER_H_

#include <list>
#include <memory>
#include <utility>
#include <vector>

#include "base/single_thread_task_runner.h"

#include "common/suggestion/suggestion_callback.h"
#include "common/suggestion/suggestion_item.h"
#include "common/suggestion/suggestion_provider.h"

namespace opera {

class SuggestionTokens;

class SuggestionManager {
 public:
  /**
   * Create a SuggestionManager instance.
   *
   * @param message_loop message loop responsible for running queries.
   */
  explicit SuggestionManager(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);

  ~SuggestionManager();

  /**
   * Add a suggestion provider to the suggestion manager to receive
   * calls to its Query method.
   *
   * @param provider the provider to be added.
   * @param type the type identifier that will be associated with all suggestion
   * items that are passed back from the given provider.
   */
  void AddProvider(std::unique_ptr<SuggestionProvider> provider,
                   const std::string& type);

  /**
  * Performs an asynchronous query on all added providers by queuing
  * calls to SuggestionProvider::Query.
  *
  * @param query the query used to generate suggestions. Can be empty.
  * @param private_browsing indicates if we're in a private browsing context.
  * @param callback the callback that will receive suggestions.
  */
  void Query(const SuggestionTokens& query,
             bool private_browsing,
             const SuggestionCallback& callback);

  /**
  * Cancel a pending query. After a query has been cancelled, there
  * will be no more calls to the callback for the pending query. Note
  * that if Cancel is called before all calls to
  * SuggestionProvider::Query have completed, those will still be
  * called, but their result will be filtered by the suggestion
  * manager.
  */
  void Cancel();

  /**
   * Set allow partial result. If false, a query will only generate suggestions
   * after all providers has returned. So a query callback will always return the
   * full list of suggestions - but delayed.
   * Default value is true, so query might call callback once per provider
   */
  void SetAllowPartialResult(bool allow_partial_result);

 private:
  typedef std::list<std::pair<SuggestionProvider*, const std::string> >
      SuggestionProviders;

  void HandleSuggestions(const SuggestionCallback& callback,
                         int query_id,
                         int provider_id,
                         const std::vector<SuggestionItem>& suggestions);

  void QueryInternal(SuggestionProvider* provider,
                     const SuggestionTokens& query,
                     int query_id,
                     bool private_browsing,
                     const std::string& type,
                     const SuggestionCallback& callback);

  void CancelInternal();

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  SuggestionProviders providers_;

  std::list<std::pair<int, SuggestionItem> > suggestions_;

  int current_query_id_;

  int pending_provider_queries_;

  bool allow_partial_result_;

  DISALLOW_COPY_AND_ASSIGN(SuggestionManager);
};

}  // namespace opera

#endif  // COMMON_SUGGESTION_SUGGESTION_MANAGER_H_
