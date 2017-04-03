// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/suggestions/op_suggestion_manager.h"

#include <string>
#include <vector>

#include "base/bind.h"

#include "common/suggestion/suggestion_provider.h"
#include "common/suggestion/suggestion_tokens.h"
#include "common/suggestions/op_suggestion_provider.h"

OpSuggestionManager::OpSuggestionManager(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : opera::SuggestionManager(task_runner) {
}

void OpSuggestionManager::AddProvider(opera::SuggestionProvider* provider,
                                      const std::string& type) {
  std::unique_ptr<opera::SuggestionProvider> prov(provider);
  SuggestionManager::AddProvider(std::move(prov), type);
}

void OpSuggestionManager::Query(
    const std::string& phrase, bool private_browsing, OpRunnable callback) {
  // Request suggestions from the manager.
  SuggestionManager::Query(phrase,
                           private_browsing,
                           base::Bind(&OpSuggestionManager::HandleSuggestions,
                                      base::Unretained(this),
                                      callback));
}

void OpSuggestionManager::HandleSuggestions(
    OpRunnable callback,
    const std::vector<opera::SuggestionItem>& suggestions) {
  callback.Run(OpSuggestionArguments(suggestions));
}
