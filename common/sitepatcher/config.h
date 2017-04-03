// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SITEPATCHER_CONFIG_H_
#define COMMON_SITEPATCHER_CONFIG_H_

// TODO(felixe): Rename this file and make these values into constants. I see
// no reason for them to differ between projects in the site patch local prefs
// store.

#define PREF_LAST_CHECK "update.last_check"

#define PREF_PREFS_OVERRIDE_SERVER "update.prefs_override.server"
#define PREF_PREFS_OVERRIDE_LOCAL "update.prefs_override.local"

#define PREF_BROWSER_JS_SERVER "update.browser_js.server"
#define PREF_BROWSER_JS_LOCAL "update.browser_js.local"

#define PREF_WEB_BLUETOOTH_BLACKLIST_SERVER \
  "update.web_bluetooth_blacklist.server"
#define PREF_WEB_BLUETOOTH_BLACKLIST_LOCAL \
  "update.web_bluetooth_blacklist.local"

#endif  // COMMON_SITEPATCHER_CONFIG_H_
