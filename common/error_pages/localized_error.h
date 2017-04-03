// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/common/localized_error.h
// @final-synchronized 135d586ae43b9260ef2ac198d5eddc158a5743fc

#ifndef COMMON_ERROR_PAGES_LOCALIZED_ERROR_H_
#define COMMON_ERROR_PAGES_LOCALIZED_ERROR_H_

#include <string>

#include "base/macros.h"
#include "base/strings/string16.h"

class GURL;

namespace base {
class DictionaryValue;
}

namespace extensions {
class Extension;
}

namespace blink {
struct WebURLError;
}

class LocalizedError {
 public:
  // Fills |error_strings| with values to be used to build an error page used
  // on HTTP errors, like 404 or connection reset.
  static void GetStrings(const blink::WebURLError& error,
                         base::DictionaryValue* strings,
                         const std::string& locale);

  // Returns a description of the encountered error.
  static base::string16 GetErrorDetails(const blink::WebURLError& error);

  // Returns true if an error page exists for the specified parameters.
  static bool HasStrings(const std::string& error_domain, int error_code);

  // Fills |error_strings| with values to be used to build an error page which
  // warns against reposting form data. This is special cased because the form
  // repost "error page" has no real error associated with it, and doesn't have
  // enough strings localized to meaningfully fill the net error template.
  static void GetFormRepostStrings(const GURL& display_url,
                                   base::DictionaryValue* error_strings);

  static const char kHttpErrorDomain[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(LocalizedError);
};

#endif  // COMMON_ERROR_PAGES_LOCALIZED_ERROR_H_
