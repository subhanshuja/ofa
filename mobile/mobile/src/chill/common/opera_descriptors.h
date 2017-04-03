// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/android/shell_descriptors.h
// @final-synchronized

#ifndef CHILL_COMMON_OPERA_DESCRIPTORS_H_
#define CHILL_COMMON_OPERA_DESCRIPTORS_H_

#include "content/public/common/content_descriptors.h"

// This is a list of global descriptor keys to be used with the
// base::GlobalDescriptors object (see base/posix/global_descriptors.h)
enum {
  kOperaPakDescriptor = kContentIPCDescriptorMax + 1,
  kLocalePakDescriptor,
  kMinidumpDescriptor,
};

#endif  // CHILL_COMMON_OPERA_DESCRIPTORS_H_
