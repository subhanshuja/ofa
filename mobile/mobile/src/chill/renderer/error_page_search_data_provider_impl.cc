// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/renderer/error_page_search_data_provider_impl.h"

#include "chill/common/opera_messages.h"

namespace opera {

namespace {
const char kOperaErrorPageName[] = "OperaErrorPage";
}

// static
ErrorPageSearchDataProviderImpl*
ErrorPageSearchDataProviderImpl::GetInstance() {
  return base::Singleton<ErrorPageSearchDataProviderImpl>::get();
}

// static
ErrorPageSearchDataProvider* ErrorPageSearchDataProvider::GetInstance() {
  return ErrorPageSearchDataProviderImpl::GetInstance();
}

// static
std::string ErrorPageSearchDataProviderImpl::GetErrorPageName() {
  return kOperaErrorPageName;
}

ErrorPageSearchDataProviderImpl::ErrorPageSearchDataProviderImpl() {}

ErrorPageSearchDataProviderImpl::~ErrorPageSearchDataProviderImpl() {}

void ErrorPageSearchDataProviderImpl::GetStrings(
    base::DictionaryValue* strings) {
  std::string search_url = "javascript:chrome.search("
      "document.getElementsByName('q')[0].value, \"" + GetErrorPageName() +
      "\");";
  strings->SetString("searchURL", search_url);
}

}  // namespace opera
