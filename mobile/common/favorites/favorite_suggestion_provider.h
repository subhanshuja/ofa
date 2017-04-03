// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_FAVORITE_SUGGESTION_PROVIDER_H_
#define MOBILE_COMMON_FAVORITES_FAVORITE_SUGGESTION_PROVIDER_H_

#include <string>

#include "common/suggestion/suggestion_provider.h"

namespace opera {
class SuggestionTokens;
}

namespace mobile {

class Favorites;
struct TitleMatch;

class FavoriteSuggestionProvider : public opera::SuggestionProvider {
 public:
  FavoriteSuggestionProvider(Favorites* favorites);
  virtual ~FavoriteSuggestionProvider() {}

  void Query(
      const opera::SuggestionTokens& query,
      bool private_browsing,
      const std::string& type,
      const opera::SuggestionCallback& callback) override;

  void Cancel() override;

 private:
  static int CalculateScore(const TitleMatch& match);

  Favorites* favorites_;
};

}  // namespace mobile

#endif  // MOBILE_COMMON_FAVORITES_FAVORITE_SUGGESTION_PROVIDER_H_
