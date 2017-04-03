// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_ERROR_PAGES_ERROR_PAGE_SEARCH_DATA_PROVIDER_H_
#define COMMON_ERROR_PAGES_ERROR_PAGE_SEARCH_DATA_PROVIDER_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/values.h"
#include "content/public/renderer/render_thread_observer.h"

namespace opera {

/**
 * Acts as an observer for the default search provider, providing default
 * search data to the render process when generating error pages.
 */
class ErrorPageSearchDataProvider : public content::RenderThreadObserver {
 public:
  ~ErrorPageSearchDataProvider() override {}

  /** Returns the singleton implementation instance. */
  static ErrorPageSearchDataProvider* GetInstance();

  /** Sets default search data on provided @p strings argument. */
  virtual void GetStrings(base::DictionaryValue* strings) = 0;
};

}  // namespace opera

#endif  // COMMON_ERROR_PAGES_ERROR_PAGE_SEARCH_DATA_PROVIDER_H_
