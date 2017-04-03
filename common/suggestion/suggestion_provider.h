// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SUGGESTION_SUGGESTION_PROVIDER_H_
#define COMMON_SUGGESTION_SUGGESTION_PROVIDER_H_

#include <string>

#include "common/suggestion/suggestion_callback.h"

namespace opera {

class SuggestionTokens;

/**
 * Pure virtual interface for suggestion providers.
 */
class SuggestionProvider {
 public:
  virtual ~SuggestionProvider() {}

  /**
  * Query a provider for suggestions. As long as
  * SuggestionProvider::Cancel has not been called a provider is free
  * to call the provided callback multiple times. Called by
  * SuggestionManager::Query. Query will be called on the UI thread,
  * and the operations performed should be asynchronous if they are
  * not instantaneous.
  *
  * @param query the query used to generate suggestions. Can be empty.
  * @param private_browsing indicates if we're in a private browsing context.
  * @param type the type identifier that should be associated with all items
  * that are passed back in the callback.
  * @param callback the callback that will receive suggestions.
  */
  virtual void Query(const SuggestionTokens& query,
                     bool private_browsing,
                     const std::string& type,
                     const SuggestionCallback& callback) = 0;

  /**
  * Informs the provider that it can cancel its current query since
  * the result will be discarded.
  */
  virtual void Cancel() = 0;
};

}  // namespace opera

#endif  // COMMON_SUGGESTION_SUGGESTION_PROVIDER_H_
