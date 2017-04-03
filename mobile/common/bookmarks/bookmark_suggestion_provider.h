// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_BOOKMARKS_BOOKMARK_SUGGESTION_PROVIDER_H_
#define MOBILE_COMMON_BOOKMARKS_BOOKMARK_SUGGESTION_PROVIDER_H_

#include <string>

#include "common/suggestion/suggestion_callback.h"
#include "common/suggestion/suggestion_provider.h"
#include "common/suggestion/suggestion_tokens.h"

namespace bookmarks {
class BookmarkMatch;
class BookmarkModel;
}

namespace mobile {

class BookmarkSuggestionProvider : public opera::SuggestionProvider {
 public:
  BookmarkSuggestionProvider(bookmarks::BookmarkModel* model);
  virtual ~BookmarkSuggestionProvider() {}

  virtual void Query(const opera::SuggestionTokens& query,
                     bool private_browsing,
                     const std::string& type,
                     const opera::SuggestionCallback& callback) override;

  void Cancel() override;

 private:
  int CalculateScore(const bookmarks::BookmarkMatch* match);

  bookmarks::BookmarkModel* model_;
};

}  // namespace mobile

#endif  // MOBILE_COMMON_BOOKMARKS_BOOKMARK_SUGGESTION_PROVIDER_H_
