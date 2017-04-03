// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSER_PROCESS_H_
#define CHROME_BROWSER_BROWSER_PROCESS_H_

namespace network_time {
class NetworkTimeTracker;
}

class ProfileManager;

class BrowserProcess {
 public:
  BrowserProcess() { }
  virtual ~BrowserProcess() { }

  virtual ProfileManager *profile_manager() = 0;

  virtual network_time::NetworkTimeTracker* network_time_tracker() = 0;
 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserProcess);
};

extern BrowserProcess* g_browser_process;

#endif  // CHROME_BROWSER_BROWSER_PROCESS_H_
