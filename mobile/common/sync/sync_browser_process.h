// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_SYNC_SYNC_BROWSER_PROCESS_H_
#define MOBILE_COMMON_SYNC_SYNC_BROWSER_PROCESS_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "chrome/browser/browser_process.h"

class PrefRegistry;
class PrefService;
class ProfileManager;

namespace net {
class URLRequestContextGetter;
}

namespace network_time {
class NetworkTimeTracker;
}

namespace mobile {

class SyncProfile;

class SyncBrowserProcess : public BrowserProcess {
 public:
  SyncBrowserProcess(
      std::unique_ptr<SyncProfile> profile,
      const base::FilePath& base_path,
      const scoped_refptr<base::SequencedTaskRunner>& io_task_runner);
  virtual ~SyncBrowserProcess();

  ProfileManager* profile_manager() override;

  network_time::NetworkTimeTracker* network_time_tracker() override;

 private:
  scoped_refptr<PrefRegistry> pref_registry_;
  std::unique_ptr<PrefService> pref_service_;
  std::unique_ptr<ProfileManager> profile_manager_;
  std::unique_ptr<network_time::NetworkTimeTracker> network_time_tracker_;
};

}  // namespace mobile

#endif  // MOBILE_COMMON_SYNC_SYNC_BROWSER_PROCESS_H_
