// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_FAVORITES_FAVORITE_SUGGESTION_PROVIDER_H_
#define COMMON_FAVORITES_FAVORITE_SUGGESTION_PROVIDER_H_

#include <string>

#include "common/favorites/favorite_utils.h"
#include "common/suggestion/suggestion_callback.h"
#include "common/suggestion/suggestion_provider.h"
#include "common/suggestion/suggestion_tokens.h"

namespace opera {

class FavoriteCollection;

class FavoriteSuggestionProvider : public SuggestionProvider {
 public:
  FavoriteSuggestionProvider(FavoriteCollection* collection);
  virtual ~FavoriteSuggestionProvider() {}

  virtual void Query(
      const SuggestionTokens& query,
      bool private_browsing,
      const std::string& type,
      const SuggestionCallback& callback) override;

  void Cancel() override;

 private:
  int CalculateScore(const FavoriteUtils::TitleMatch& match);

  FavoriteCollection* collection_;
};

}  // namespace opera

#endif  // COMMON_FAVORITES_FAVORITE_SUGGESTION_PROVIDER_H_
