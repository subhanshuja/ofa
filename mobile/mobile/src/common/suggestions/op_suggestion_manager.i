// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include <list>

#include "common/suggestions/op_suggestion_manager.h"
#include "common/suggestions/op_suggestion_provider.h"
#include "common/swig_utils/op_arguments.h"
#include "common/swig_utils/op_runnable.h"
%}

namespace opera {

struct SuggestionItem {
  SuggestionItem();
  SuggestionItem(
      int relevance, const std::string& title, const std::string& url,
      const std::string& type)
      : relevance(relevance), title(title), url(url), type(type);

  int relevance;
  std::string title;
  std::string url;
  std::string type;
};

}   // namespace opera

VECTOR(SuggestionItemList, opera::SuggestionItem);

class OpSuggestionArguments : public OpArguments {
 public:
  std::vector<opera::SuggestionItem> items;
};

SWIG_SELFREF_CONSTRUCTOR(OpSuggestionProvider);

%feature("director", assumeoverride=1) OpSuggestionProvider;
class OpSuggestionProvider : public opera::SuggestionProvider {
 public:
  virtual ~OpSuggestionProvider() {}
  virtual void Query(const std::string& query,
                     bool private_browsing,
                     OpRunnable callback) = 0;
  virtual void Cancel() = 0;
};

%nodefaultctor OpSuggestionManager;
class OpSuggestionManager {
 public:
  %apply SWIGTYPE* JAVADISOWN { opera::SuggestionProvider* provider };
  void AddProvider(opera::SuggestionProvider* provider,
                   const std::string& type);
  %clear opera::SuggestionProvider* provider;

  void Query(
      const std::string& query, bool private_browsing, OpRunnable callback);

  void Cancel();
};
