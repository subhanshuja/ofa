// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SUGGESTION_SUGGESTION_CALLBACK_H_
#define COMMON_SUGGESTION_SUGGESTION_CALLBACK_H_

#include <vector>

#include "base/callback.h"

#include "common/suggestion/suggestion_item.h"

namespace opera {

typedef base::Callback<void(const std::vector<SuggestionItem>&)>
    SuggestionCallback;

}  // namespace opera

#endif  // COMMON_SUGGESTION_SUGGESTION_CALLBACK_H_
