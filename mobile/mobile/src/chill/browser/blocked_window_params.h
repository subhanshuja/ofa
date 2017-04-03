// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from src/chrome/browser/ui/blocked_content/blocked_window_params.h
// @final-synchronized
#ifndef CHILL_BROWSER_BLOCKED_WINDOW_PARAMS_H_
#define CHILL_BROWSER_BLOCKED_WINDOW_PARAMS_H_

#include "url/gurl.h"

namespace opera {
class BlockedWindowParams {
 public:
  BlockedWindowParams(const GURL& target_url,
                      int render_process_id,
                      int opener_render_frame_id);

  int opener_render_frame_id() const {
    return opener_render_frame_id_;
  }

  int render_process_id() const {
    return render_process_id_;
  }

  const GURL& target_url() const {
    return target_url_;
  }

 private:
  GURL target_url_;
  int render_process_id_;
  int opener_render_frame_id_;
};

}  // namespace opera

#endif  // CHILL_BROWSER_BLOCKED_WINDOW_PARAMS_H_
