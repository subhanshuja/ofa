// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/history/snippet.h
// @final-synchronized eb018f04a183b2c5bb6784f0a0ee97abf84c6f61

// This module computes snippets of queries based on hits in the documents
// for display in history search results.

#ifndef COMMON_SUGGESTION_SNIPPET_H_
#define COMMON_SUGGESTION_SNIPPET_H_

#include <vector>

#include "base/strings/string16.h"

namespace opera {

typedef std::pair<size_t, size_t> SuggestionMatchPosition;
typedef std::vector<SuggestionMatchPosition> SuggestionMatchPositions;

}  // namespace opera

#endif  // COMMON_SUGGESTION_SNIPPET_H_
