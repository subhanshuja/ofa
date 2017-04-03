// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/persistent_tab_helper.h"
%}

namespace opera {

%newobject PersistentTabHelper::GetNavigationHistory();

%rename(NativePersistentTabHelper) PersistentTabHelper;
// Apply mixedCase to method names in Java.
%rename("%(regex:/PersistentTabHelper::([A-Z].*)/\\l\\1/)s", regextarget=1, fullname=1, %$isfunction) ".*";

class PersistentTabHelper {
 public:
  PersistentTabHelper(jobject j_web_contents);
  virtual ~PersistentTabHelper() {}

  NavigationHistory* CreateNavigationHistory();
  void SetNavigationHistory(const NavigationHistory& history);
};

}  // namespace opera
