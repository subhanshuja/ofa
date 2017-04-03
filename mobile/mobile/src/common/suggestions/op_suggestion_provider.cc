// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/suggestions/op_suggestion_provider.h"

#include "base/bind.h"
#include "base/callback.h"

OpSuggestionArguments::OpSuggestionArguments(
    const std::vector<opera::SuggestionItem> list)
    : items(list) {}

void OpSuggestionProvider::SuggestionQueryCallback(
    opera::SuggestionCallback callback, const OpArguments& args) {
  callback.Run(static_cast<const OpSuggestionArguments&>(args).items);
}

void OpSuggestionProvider::Query(const opera::SuggestionTokens& query,
                                 bool private_browsing,
                                 const std::string& type,
                                 const opera::SuggestionCallback& callback) {
  Query(query.phrase(),
        private_browsing,
        base::Bind(&OpSuggestionProvider::SuggestionQueryCallback,
                   base::Unretained(this),
                   callback));
}
