// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SITEPATCHER_BROWSER_OP_SITE_PATCHER_CONFIG_H_
#define COMMON_SITEPATCHER_BROWSER_OP_SITE_PATCHER_CONFIG_H_

#include <string>

#include "base/files/file_path.h"

struct OpSitepatcherConfig {
  std::string update_check_url;
  std::string browser_js_url;
  std::string prefs_override_url;
  std::string web_bluetooth_blacklist_url;

  base::FilePath update_prefs_file;
  base::FilePath prefs_override_file;
  base::FilePath browser_js_file;
  base::FilePath user_js_file;
  base::FilePath web_bluetooth_blacklist_file;

  double update_check_interval;
  double update_fail_interval;

  bool verify_browser_js;
  bool verify_site_prefs;
  bool verify_web_bluetooth_blacklist;

  // false if disabled
  bool sitepatcher_enabled;

  // [chromium-style]
  // Complex class/struct needs an explicit out-of-line constructor.
  OpSitepatcherConfig();
  OpSitepatcherConfig(const OpSitepatcherConfig&);
  ~OpSitepatcherConfig();
};

#endif  // COMMON_SITEPATCHER_BROWSER_OP_SITE_PATCHER_CONFIG_H_
