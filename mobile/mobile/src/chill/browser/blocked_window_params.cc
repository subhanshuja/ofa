// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from src/chrome/browser/ui/blocked_content/blocked_window_params.cc
// @final-synchronized

#include "chill/browser/blocked_window_params.h"

#include "url/gurl.h"

namespace opera {
BlockedWindowParams::BlockedWindowParams(
    const GURL& target_url,
    int render_process_id,
    int opener_render_frame_id)
    : target_url_(target_url),
      render_process_id_(render_process_id),
      opener_render_frame_id_(opener_render_frame_id) {
}
}  // namespace opera
