// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SITEPATCHER_BROWSER_OP_UPDATE_CHECKER_H_
#define COMMON_SITEPATCHER_BROWSER_OP_UPDATE_CHECKER_H_

#include "common/sitepatcher/browser/op_site_patcher_config.h"
#include "net/url_request/url_request_context_getter.h"

class PersistentPrefStore;
class OpSitePatcher;
class OpUpdateCheckerImpl;

class OpUpdateChecker {
 public:
  typedef bool (*Callback)(OpSitePatcher*);

  OpUpdateChecker(scoped_refptr<net::URLRequestContextGetter> request_context,
                  const OpSitepatcherConfig& config,
                  OpSitePatcher* parent);

  virtual ~OpUpdateChecker();

  void SetPrefs(PersistentPrefStore* prefs);

  void Start(bool force_check);

 private:
  friend class OpUpdateCheckerImpl;
  OpUpdateCheckerImpl* impl_;
};

#endif  // COMMON_SITEPATCHER_BROWSER_OP_UPDATE_CHECKER_H_
