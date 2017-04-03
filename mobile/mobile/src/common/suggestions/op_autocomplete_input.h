// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/components/omnibox/browser/autocomplete_input.h
// @final-synchronized 475c9e32d72fcb59ef281944324619d784479409

#ifndef COMMON_SUGGESTIONS_OP_AUTOCOMPLETE_INPUT_H_
#define COMMON_SUGGESTIONS_OP_AUTOCOMPLETE_INPUT_H_

#include <string>

#include "base/strings/string16.h"
#include "url/gurl.h"
#include "url/third_party/mozilla/url_parse.h"

// The user input for an autocomplete query.  Allows copying.
class AutocompleteInput {
 public:
  enum Type {
    // Empty input
    INVALID = 0,
    // Valid input whose type cannot be determined
    UNKNOWN = 1,
    // Input autodetected as UNKNOWN, which the user wants to treat as an URL
    // by specifying a desired_tld
    DEPRECATED_REQUESTED_URL = 2,
    // Input autodetected as a URL
    URL = 3,
    // Input autodetected as a query
    QUERY = 4,
    // Input is detected as a forced query
    FORCED_QUERY = 5
  };

  // Parses |text| (including an optional |desired_tld|) and returns the type of
  // input this will be interpreted as.  |scheme_classifier| is used to check
  // the scheme in |text| is known and registered in the current environment.
  // The components of the input are stored in the output parameter |parts|, if
  // it is non-NULL. The scheme is stored in |scheme| if it is non-NULL. The
  // canonicalized URL is stored in |canonicalized_url|; however, this URL is
  // not guaranteed to be valid, especially if the parsed type is, e.g., QUERY.
  static Type Parse(
      const base::string16& text,
      const std::string& desired_tld,
      url::Parsed* parts,
      base::string16* scheme,
      GURL* canonicalized_url);

  // Returns the number of non-empty components in |parts| besides the host.
  static int NumNonHostComponents(const url::Parsed& parts);

  static Type ParseAutocompleteInput(const base::string16& text);
};

#endif  // COMMON_SUGGESTIONS_OP_AUTOCOMPLETE_INPUT_H_
