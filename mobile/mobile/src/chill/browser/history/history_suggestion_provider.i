// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/history/history_suggestion_provider.h"
#include "common/suggestion/suggestion_provider.h"
%}

namespace opera {

%feature("director", assumeoverride=1) HistorySuggestionProvider;
class HistorySuggestionProvider : public opera::SuggestionProvider {
 public:
  HistorySuggestionProvider(bool exclude_favorites);
};

}  // namespace opera
