// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SUGGESTION_SUGGESTION_TOKENS_H_
#define COMMON_SUGGESTION_SUGGESTION_TOKENS_H_

#include <list>
#include <string>

namespace opera {

/// A class representing a phrase and its tokens.
class SuggestionTokens {
 public:
  /// Create an empty SuggestionTokens object.
  SuggestionTokens();

  ~SuggestionTokens();

  /// Create a SuggestionTokens object for the given phrase.
  /// @param phrase The complete phrase.
  SuggestionTokens(const std::string& phrase);  // NOLINT(runtime/explicit)

  /// @returns the complete phrase, with heading/trailing white spaces trimmed
  /// off.
  const std::string& phrase() const {
    return phrase_;
  }

  /// @returns the list of lower-case terms in the phrase.
  const std::list<std::string>& terms() const {
    return terms_;
  }

  /// Checks equality of two SuggestionTokens objects.
  /// Equality is defined as the equality of the phrases of both the objects.
  bool operator==(const SuggestionTokens& other) const {
    return phrase_ == other.phrase_;
  }

  /// Checks inequality of two SuggestionTokens objects.
  bool operator!=(const SuggestionTokens& other) const {
    return phrase_ != other.phrase_;
  }

 private:
  std::string phrase_;
  std::list<std::string> terms_;
};

}  // namespace opera

#endif  // COMMON_SUGGESTION_SUGGESTION_TOKENS_H_
