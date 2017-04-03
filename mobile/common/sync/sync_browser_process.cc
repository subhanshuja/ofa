// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/sync/sync_browser_process.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "base/logging.h"
#include "base/files/file_path.h"
#include "base/time/default_clock.h"
#include "base/time/default_tick_clock.h"

#include "components/network_time/network_time_tracker.h"
#include "components/prefs/pref_service_factory.h"
#include "components/prefs/pref_registry.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

#include "mobile/common/sync/sync_profile.h"
#include "mobile/common/sync/sync_profile_manager.h"

#include "net/url_request/url_request_context_getter.h"

BrowserProcess* g_browser_process;

namespace mobile {

SyncBrowserProcess::SyncBrowserProcess(
    std::unique_ptr<SyncProfile> profile,
    const base::FilePath& base_path,
    const scoped_refptr<base::SequencedTaskRunner>& io_task_runner) {
  DCHECK(!g_browser_process);
  g_browser_process = this;

  PrefServiceFactory factory;
  PrefRegistrySimple* registry = new PrefRegistrySimple();
  network_time::NetworkTimeTracker::RegisterPrefs(registry);
  pref_registry_ = registry;
  factory.SetUserPrefsFile(base_path.Append("time.json"), io_task_runner.get());
  pref_service_ = factory.Create(pref_registry_.get());
  profile_manager_ = base::MakeUnique<SyncProfileManager>(std::move(profile));
}

SyncBrowserProcess::~SyncBrowserProcess() {
  DCHECK(g_browser_process == this);
  g_browser_process = NULL;
}

ProfileManager* SyncBrowserProcess::profile_manager() {
  return profile_manager_.get();
}

network_time::NetworkTimeTracker* SyncBrowserProcess::network_time_tracker() {
  if (!network_time_tracker_) {
    network_time_tracker_.reset(new network_time::NetworkTimeTracker(
        base::WrapUnique(new base::DefaultClock()),
        base::WrapUnique(new base::DefaultTickClock()),
        pref_service_.get(),
        scoped_refptr<net::URLRequestContextGetter>()));
  }
  return network_time_tracker_.get();
}

}  // namespace mobile
