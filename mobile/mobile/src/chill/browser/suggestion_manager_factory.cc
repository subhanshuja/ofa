// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/suggestion_manager_factory.h"

#include "content/public/browser/browser_thread.h"

#include "common/suggestions/op_suggestion_manager.h"

namespace opera {

OpSuggestionManager* SuggestionManagerFactory::CreateSuggestionManager() {
  return new OpSuggestionManager(
      content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::UI));
}

}  // namespace opera
