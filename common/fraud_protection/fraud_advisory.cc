// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/fraud_protection/fraud_advisory.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/icu/source/i18n/unicode/regex.h"

namespace opera {

FraudAdvisory::FraudAdvisory()
    : type_(ADVISORY_TYPE_UNKNOWN) {
}

FraudAdvisory::FraudAdvisory(const FraudAdvisory&) = default;

FraudAdvisory::~FraudAdvisory() {
  for (FraudDetectors::iterator iter = detectors_.begin();
       iter != detectors_.end();
       ++iter) {
    delete *iter;
  }
}

bool FraudAdvisory::IsValid() const {
  return !detectors_.empty() && type_ != ADVISORY_TYPE_UNKNOWN;
}

bool FraudAdvisory::IsFraudlentUrl(const GURL& url) const {
  for (FraudDetectors::const_iterator iter = detectors_.begin();
       iter != detectors_.end();
       ++iter) {
    if ((*iter)->IsFraudlentUrl(url)) {
      return true;
    }
  }

  return false;
}

void FraudAdvisory::AddHostBasedFraud(const std::string& host_url) {
  detectors_.push_back(new HostFraudDetector(host_url));
}

void FraudAdvisory::AddRegExBasedFraud(const std::string& regexp) {
  detectors_.push_back(new RegExDetector(regexp));
}

bool FraudAdvisory::HostFraudDetector::IsFraudlentUrl(const GURL& url) const {
  GURL::Replacements replacements;
  replacements.ClearUsername();
  replacements.ClearPassword();

  GURL clean_url = url.ReplaceComponents(replacements);
  const auto size = template_.size();
  return base::StartsWith(
      clean_url.spec(),
      base::StringPiece(
          template_.c_str(),
          template_.empty() || template_[size - 1] != '/' ? size : size - 1),
      base::CompareCase::INSENSITIVE_ASCII);
}

bool FraudAdvisory::RegExDetector::IsFraudlentUrl(const GURL& url) const {
  icu::UnicodeString regex(template_.c_str());
  icu::UnicodeString match_url(url.spec().c_str());
  UErrorCode status = U_ZERO_ERROR;

  icu::RegexMatcher matcher(regex, match_url, UREGEX_CASE_INSENSITIVE, status);
  return matcher.lookingAt(status) == TRUE;
}

}  // namespace opera
