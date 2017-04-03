// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/suggestion_manager_factory.h"
%}

namespace opera {

%nodefaultctor SuggestionManagerFactory;
%nodefaultdtor SuggestionManagerFactory;
%newobject SuggestionManagerFactory::CreateSuggestionManager();
class SuggestionManagerFactory {
 public:
  static OpSuggestionManager* CreateSuggestionManager();
};

}  // namespace opera
