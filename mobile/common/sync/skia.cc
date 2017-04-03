/*
 * Copyright 2008 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkBitmap.h"

#include <cstdlib>

void SkDebugf_FileLine(const char* file,
                       int line,
                       bool fatal,
                       const char* format,
                       ...) {
}

SkBitmap::SkBitmap() {
  sk_bzero(this, sizeof(*this));
}

SkBitmap::SkBitmap(const SkBitmap& src) {
  sk_bzero(this, sizeof(*this));
}

SkBitmap::~SkBitmap() {
}

void sk_abort_no_print() {
  abort();
}
