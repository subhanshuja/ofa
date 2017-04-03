// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sitepatcher/browser/op_site_patcher_config.h"

OpSitepatcherConfig::OpSitepatcherConfig()
    : update_check_interval(1.0),
      update_fail_interval(1.0),
      verify_browser_js(true),
      verify_site_prefs(false),
      verify_web_bluetooth_blacklist(true),
      sitepatcher_enabled(true) {}

OpSitepatcherConfig::OpSitepatcherConfig(const OpSitepatcherConfig&) = default;

OpSitepatcherConfig::~OpSitepatcherConfig() {}
