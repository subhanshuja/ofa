// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_ERROR_PAGES_ERROR_PAGE_H_
#define COMMON_ERROR_PAGES_ERROR_PAGE_H_

#include <string>

#include "base/strings/string16.h"

#include "third_party/WebKit/public/platform/WebURLError.h"
#include "third_party/WebKit/public/platform/WebURLRequest.h"

void GenerateErrorPage(const blink::WebURLRequest& failed_request,
                       const blink::WebURLError& error,
                       std::string* error_html,
                       base::string16* error_description);

#endif  // COMMON_ERROR_PAGES_ERROR_PAGE_H_
