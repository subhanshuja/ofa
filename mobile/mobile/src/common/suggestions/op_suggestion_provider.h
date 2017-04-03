// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SUGGESTIONS_OP_SUGGESTION_PROVIDER_H_
#define COMMON_SUGGESTIONS_OP_SUGGESTION_PROVIDER_H_

#include <list>
#include <string>
#include <vector>

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "common/suggestion/suggestion_callback.h"
#include "common/suggestion/suggestion_item.h"
#include "common/suggestion/suggestion_provider.h"
#include "common/suggestion/suggestion_tokens.h"

#include "common/swig_utils/op_arguments.h"
#include "common/swig_utils/op_runnable.h"

class OpSuggestionArguments : public OpArguments {
 public:
  explicit OpSuggestionArguments(std::vector<opera::SuggestionItem> list);
  OpSuggestionArguments() {}
  std::vector<opera::SuggestionItem> items;
 private:
  SWIG_CLASS_NAME
};

class OpSuggestionProvider
    : public opera::SuggestionProvider,
      public base::android::ScopedJavaGlobalRef<jobject> {
 public:
  OpSuggestionProvider() {}
  virtual ~OpSuggestionProvider() {}
  virtual void Query(const std::string& query,
                     bool private_browsing,
                     OpRunnable callback) = 0;
  virtual void Cancel() = 0;

  virtual void Query(const opera::SuggestionTokens& query,
                     bool private_browsing,
                     const std::string& type,
                     const opera::SuggestionCallback& callback);
 private:
  void SuggestionQueryCallback(
      opera::SuggestionCallback callback, const OpArguments& args);
};

#endif  // COMMON_SUGGESTIONS_OP_SUGGESTION_PROVIDER_H_
