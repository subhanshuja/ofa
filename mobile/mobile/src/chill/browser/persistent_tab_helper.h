// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_PERSISTENT_TAB_HELPER_H_
#define CHILL_BROWSER_PERSISTENT_TAB_HELPER_H_

#include <string>
#include <jni.h>

#include "content/public/browser/web_contents.h"

namespace content {
class WebContents;
}  // namespace content

namespace opera {

class NavigationHistory;

class PersistentTabHelper {
 public:
  explicit PersistentTabHelper(jobject j_web_contents);
  virtual ~PersistentTabHelper() {}

  // Restores the associated WebContents based on the contents of a
  // NavigationHistory.
  void SetNavigationHistory(const NavigationHistory& history);

  // Create a snapshot of the associated WebContents history.
  NavigationHistory* CreateNavigationHistory();

 private:
  content::WebContents* web_contents_;
};

}  // namespace opera

#endif  // CHILL_BROWSER_PERSISTENT_TAB_HELPER_H_
