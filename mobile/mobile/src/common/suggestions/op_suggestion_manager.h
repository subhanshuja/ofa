// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SUGGESTIONS_OP_SUGGESTION_MANAGER_H_
#define COMMON_SUGGESTIONS_OP_SUGGESTION_MANAGER_H_

#include <string>
#include <vector>

#include "base/single_thread_task_runner.h"

#include "common/suggestion/suggestion_item.h"
#include "common/suggestion/suggestion_manager.h"
#include "common/suggestion/suggestion_provider.h"

#include "common/swig_utils/op_arguments.h"
#include "common/swig_utils/op_runnable.h"

class OpSuggestionProvider;

class OpSuggestionManager : public opera::SuggestionManager {
 public:
  explicit OpSuggestionManager(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);

  void AddProvider(opera::SuggestionProvider* provider,
                   const std::string& type);

  void Query(
      const std::string& phrase, bool private_browsing, OpRunnable callback);

 private:
  void HandleSuggestions(OpRunnable callback,
                         const std::vector<opera::SuggestionItem>& suggestions);
};

#endif  // COMMON_SUGGESTIONS_OP_SUGGESTION_MANAGER_H_
