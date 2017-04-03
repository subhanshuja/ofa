// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_FIND_IN_PAGE_H_
#define CHILL_BROWSER_FIND_IN_PAGE_H_

#include <jni.h>

#include "base/strings/string16.h"

namespace content {
class WebContents;
}  // namespace content

namespace opera {

class FindInPage {
 public:
  explicit FindInPage(jobject web_contents);

  void Find(int request_id,
            const base::string16& term,
            bool forward,
            bool match_case,
            bool find_next);
  void Cancel();

 private:
  content::WebContents* web_contents_;
};

}  // namespace opera

#endif  // CHILL_BROWSER_FIND_IN_PAGE_H_
