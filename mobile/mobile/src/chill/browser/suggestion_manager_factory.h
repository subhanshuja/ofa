// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_SUGGESTION_MANAGER_FACTORY_H_
#define CHILL_BROWSER_SUGGESTION_MANAGER_FACTORY_H_

#include "base/macros.h"

class OpSuggestionManager;

namespace opera {

class SuggestionManagerFactory {
 public:
  static OpSuggestionManager* CreateSuggestionManager();

 private:
  DISALLOW_COPY_AND_ASSIGN(SuggestionManagerFactory);
};

}  // namespace opera

#endif  // CHILL_BROWSER_SUGGESTION_MANAGER_FACTORY_H_
