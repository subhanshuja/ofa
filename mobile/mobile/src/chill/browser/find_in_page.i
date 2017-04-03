// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/find_in_page.h"
%}

namespace opera {

%rename(NativeFindInPage) FindInPage;

class FindInPage {
 public:
  explicit FindInPage(jobject web_contents);

  void FindInPage::Find(int request_id,
                        const base::string16& term,
                        bool forward,
                        bool match_case,
                        bool find_next);
  void Cancel();
};

}  // namespace opera
