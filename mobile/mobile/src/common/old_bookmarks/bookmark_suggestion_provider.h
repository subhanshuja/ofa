// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_SUGGESTION_PROVIDER_H_
#define MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_SUGGESTION_PROVIDER_H_

#include <string>

#include "common/old_bookmarks/bookmark_utils.h"
#include "common/suggestion/suggestion_callback.h"
#include "common/suggestion/suggestion_provider.h"
#include "common/suggestion/suggestion_tokens.h"

namespace opera {

class BookmarkCollection;

class BookmarkSuggestionProvider : public SuggestionProvider {
 public:
  explicit BookmarkSuggestionProvider(BookmarkCollection* collection);
  virtual ~BookmarkSuggestionProvider() {}

  virtual void Query(const SuggestionTokens& query,
                     bool private_browsing,
                     const std::string& type,
                     const SuggestionCallback& callback) override;

  void Cancel() override;

 private:
  int CalculateScore(const BookmarkUtils::TitleMatch& match);

  BookmarkCollection* collection_;
};

}  // namespace opera

#endif  // MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_SUGGESTION_PROVIDER_H_
