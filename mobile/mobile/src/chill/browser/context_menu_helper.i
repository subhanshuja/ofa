// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/context_menu_helper.h"
%}

%feature("director") opera::ContextMenuHelper;
SWIG_SELFREF_NAMESPACED_CONSTRUCTOR(opera, ContextMenuHelper);

namespace opera {

%rename(NativeContextMenuHelper) ContextMenuHelper;
%rename("%(lowercamelcase)s") "ContextMenuHelper::ShowContextMenu";

class ContextMenuHelper {
 public:
  ContextMenuHelper(jobject j_web_contents);
  virtual ~ContextMenuHelper() {}

  virtual void ShowContextMenu(blink::WebContextMenuData::MediaType media_type,
                               std::string link_url,
                               std::string link_text,
                               std::string unfiltered_link_url,
                               std::string src_url,
                               std::string keyword_url,
                               std::string selection_text,
                               bool is_editable,
                               std::string sanitized_referrer,
                               blink::WebReferrerPolicy referrer_policy,
                               int x,
                               int y,
                               std::string title) = 0;
};

}  // namespace opera

