// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_RENDERER_ERROR_PAGE_SEARCH_DATA_PROVIDER_IMPL_H_
#define CHILL_RENDERER_ERROR_PAGE_SEARCH_DATA_PROVIDER_IMPL_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/memory/singleton.h"

#include "common/error_pages/error_page_search_data_provider.h"

namespace opera {

/**
 * Acts as an observer for the default search provider, providing default
 * search data to the render process when generating error pages.
 */
class ErrorPageSearchDataProviderImpl : public ErrorPageSearchDataProvider {
 public:
  static ErrorPageSearchDataProviderImpl* GetInstance();
  static std::string GetErrorPageName();

  // ErrorPageSearchDataProvider implementation:
  void GetStrings(base::DictionaryValue* strings) override;

 private:
  friend struct base::DefaultSingletonTraits<ErrorPageSearchDataProviderImpl>;

  ErrorPageSearchDataProviderImpl();
  ~ErrorPageSearchDataProviderImpl() override;

  DISALLOW_COPY_AND_ASSIGN(ErrorPageSearchDataProviderImpl);
};

}  // namespace opera

#endif  // CHILL_RENDERER_ERROR_PAGE_SEARCH_DATA_PROVIDER_IMPL_H_
