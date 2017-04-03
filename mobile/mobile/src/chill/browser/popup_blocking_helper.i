// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/popup_blocking_helper.h"
%}

%feature("director") opera::PopupBlockingHelper;
SWIG_SELFREF_NAMESPACED_CONSTRUCTOR(opera, PopupBlockingHelper);

namespace opera {

%rename(NativePopupBlockingHelper) PopupBlockingHelper;
%rename("%(lowercamelcase)s") "PopupBlockingHelper::OnPopupBlocked";

class PopupBlockingHelper {
 public:
  PopupBlockingHelper(jobject j_web_contents);
  virtual ~PopupBlockingHelper() {}

  virtual void OnPopupBlocked(const GURL& url) = 0;
};

}  // namespace opera

