// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/persistent_tab_helper.h"

#include <vector>

#include "base/android/jni_android.h"
#include "content/public/browser/reload_type.h"

#include "chill/browser/navigation_history.h"

namespace opera {

PersistentTabHelper::PersistentTabHelper(jobject j_web_contents)
    : web_contents_(content::WebContents::FromJavaWebContents(
          base::android::ScopedJavaLocalRef<jobject>(
              base::android::AttachCurrentThread(),
              j_web_contents))) {}

NavigationHistory* PersistentTabHelper::CreateNavigationHistory() {
  NavigationHistory* history = new NavigationHistory();
  const content::NavigationController& controller =
      web_contents_->GetController();

  int count = controller.GetEntryCount();
  for (int i = 0; i < count; ++i)
    history->AppendEntry(controller.GetEntryAtIndex(i));

  history->set_current_entry(controller.GetCurrentEntryIndex());

  return history;
}

void PersistentTabHelper::SetNavigationHistory(
    const NavigationHistory& history) {
  // content::NavigationController::Restore takes ownership of the entries and
  // clears the vector. Create a copy before calling it.
  std::vector<std::unique_ptr<content::NavigationEntry>> entries;
  for (std::vector<content::NavigationEntry*>::const_iterator i =
           history.entries().begin();
       i != history.entries().end(); ++i) {
    content::NavigationEntryImpl* impl =
        content::NavigationEntryImpl::FromNavigationEntry(*i);
    entries.push_back(impl->Clone());
  }

  web_contents_->GetController().Restore(
      history.current_entry(), content::RestoreType::CURRENT_SESSION, &entries);
}

}  // namespace opera
