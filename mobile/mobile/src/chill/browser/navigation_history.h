// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_NAVIGATION_HISTORY_H_
#define CHILL_BROWSER_NAVIGATION_HISTORY_H_

#include <vector>

#include "base/memory/ref_counted.h"
#include "content/browser/frame_host/navigation_entry_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_entry.h"

namespace opera {

class NavigationHistory
    : public base::RefCountedThreadSafe<
          NavigationHistory,
          content::BrowserThread::DeleteOnUIThread> {
 public:
  NavigationHistory()
      : current_entry_(0) {
  }

  ~NavigationHistory() {
    DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
    for (std::vector<content::NavigationEntry*>::iterator i = entries_.begin();
        i != entries_.end(); ++i) {
      delete *i;
    }
  }

  void AppendEntry(content::NavigationEntry* entry) {
    content::NavigationEntryImpl* impl =
        content::NavigationEntryImpl::FromNavigationEntry(entry);
    content::NavigationEntryImpl* clone = impl->Clone().release();
    // WAM-6906: Preserve the unique identifier, is't used as key for
    // mOperaPageMap in com.opera.android.browser.Tab
    clone->set_unique_id(entry->GetUniqueID());
    entries_.push_back(clone);
  }

  content::NavigationEntry* AppendNewEntry() {
    entries_.push_back(content::NavigationEntry::Create().release());
    return entries_.back();
  }

  int current_entry() const {
    return current_entry_;
  }

  void set_current_entry(int current_entry) {
    current_entry_ = current_entry;
  }

  const std::vector<content::NavigationEntry*>& entries() const {
    return entries_;
  }

 private:
  int current_entry_;
  std::vector<content::NavigationEntry*> entries_;
};

}  // namespace opera

#endif  // CHILL_BROWSER_NAVIGATION_HISTORY_H_
