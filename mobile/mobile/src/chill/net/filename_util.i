// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//

%{
#include "net/base/filename_util.h"
%}

#include <string>

#include "base/strings/string16.h"

class GURL;

namespace  net {

base::string16 GetSuggestedFilename(
    const GURL& url,
    const std::string& content_disposition,
    const std::string& referrer_charset,
    const std::string& suggested_name,
    const std::string& mime_type,
    const std::string& default_name);

}  // namespace net
