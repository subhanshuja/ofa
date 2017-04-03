// Copyright (c) 2012-2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/public/common/content_paths.h
// @final-synchronized

#ifndef CHILL_COMMON_PATHS_H_
#define CHILL_COMMON_PATHS_H_

// This file declares path keys for opera.  These can be used with
// the PathService to access various special directories and files.

namespace opera {

class Paths {
 public:
  enum {
    // Various parts of chromium seem to start on some thousand, so let's start
    // somewhere else and hope for the best.
    PATH_START = 500,
    BREAKPAD_CRASH_DUMP,
    PATH_END
  };
};

}  // namespace opera

#endif  // CHILL_COMMON_PATHS_H_
