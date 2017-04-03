// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_TITLE_MATCH_H_
#define MOBILE_COMMON_FAVORITES_TITLE_MATCH_H_

#include "common/suggestion/snippet.h"

namespace mobile {

class Favorite;

struct TitleMatch {
  TitleMatch();
  Favorite* node;
  // Location of the matching words in the title of the node.
  opera::SuggestionMatchPositions positions;
};

}  // namespace mobile

#endif  // MOBILE_COMMON_FAVORITES_TITLE_MATCH_H_
