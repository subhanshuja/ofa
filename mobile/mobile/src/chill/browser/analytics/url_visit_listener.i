// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/analytics/url_visit_listener.h"
%}

%feature("director") opera::URLVisitListener;

namespace opera {

%rename(NativeURLVisitListener) URLVisitListener;
%rename("%(lowercamelcase)s") "URLVisitListener::OnURLVisited";

class URLVisitListener {
 public:
  URLVisitListener();
  virtual ~URLVisitListener();

  virtual void OnURLVisited(const std::string& url, bool is_redirect) = 0;
};

}  // namespace opera
