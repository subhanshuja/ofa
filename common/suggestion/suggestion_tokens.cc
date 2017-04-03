// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/suggestion/suggestion_tokens.h"

#include "base/i18n/break_iterator.h"
#include "base/i18n/case_conversion.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"

namespace {

struct QuotePair {
  base::char16 begin;
  base::char16 end;
};

static const QuotePair kQuotationMarks[] = {
  {0x0022, 0x0022},   // QUOTATION MARK (" ")
  {0x00AB, 0x00BB},   // LEFT/RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK (« »)
  {0x2018, 0x2019},   // LEFT/RIGHT SINGLE QUOTATION MARK (‘ ’)
  {0x201C, 0x201D}    // LEFT/RIGHT DOUBLE QUOTATION MARK (“ ”)
};

void TokenizePhrase(
    const base::string16& phrase, std::list<std::string>& terms) {
  // Set up a word break iterator.
  base::i18n::BreakIterator iter(
        phrase, base::i18n::BreakIterator::BREAK_WORD);
  if (!iter.Init()) {
    return;
  }

  // Extract query tokens.
  base::char16 quote_char = 0;
  size_t quote_start = 0;
  while (iter.Advance()) {
    base::string16 str = iter.GetString();
    DCHECK(!str.empty());

    if (quote_char != 0) {
      // End of quoted string?
      if (str.length() == 1 && str[0] == quote_char) {
        size_t quote_len = iter.pos() - quote_start - 1;
        base::string16 term = phrase.substr(quote_start, quote_len);
        terms.push_back(base::UTF16ToUTF8(base::i18n::ToLower(term)));
        quote_char = 0;
      }
    } else {
      if (iter.IsWord()) {
        // We got an un-quoted term.
        terms.push_back(base::UTF16ToUTF8(base::i18n::ToLower(str)));
      } else if (str.length() == 1) {
        // Start of a quoted string?
        base::char16 c = str[0];
        for (size_t i = 0;
             i < sizeof(kQuotationMarks) / sizeof(QuotePair);
             ++i) {
          if (kQuotationMarks[i].begin == c) {
            quote_char = kQuotationMarks[i].end;
            quote_start = iter.pos();
            break;
          }
        }
      }
    }
  }
}

}  // anonymous namespace

namespace opera {

SuggestionTokens::SuggestionTokens() {
}

SuggestionTokens::~SuggestionTokens() {
}

SuggestionTokens::SuggestionTokens(const std::string& phrase) {
  // Trim head/tail spaces.
  base::string16 phrase16;
  base::UTF8ToUTF16(phrase.c_str(), phrase.size(), &phrase16);
  TrimWhitespace(phrase16, base::TRIM_ALL, &phrase16);
  phrase_ = base::UTF16ToUTF8(phrase16);

  // Tokenize the phrase.
  TokenizePhrase(phrase16, terms_);
}

}  // namespace opera
