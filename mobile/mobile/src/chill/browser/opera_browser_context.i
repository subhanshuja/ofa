// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/opera_browser_context.h"
%}

namespace opera {

class OperaBrowserContext : public content::BrowserContext {
public:
  static content::DownloadManager* GetDownloadManager(OperaBrowserContext* browser_context);

  // Opera extensions
  static void ClearBrowsingData();
  static void ClearPrivateBrowsingData();
  static void FlushCookieStorage();
  static void OnAppDestroy();
  static OperaBrowserContext* GetDefaultBrowserContext();
  static OperaBrowserContext* GetPrivateBrowserContext();
  static void SetResourceDispatcherHostDelegate(
      OperaResourceDispatcherHostDelegate* delegate);

  static void SetTurboDelegate(TurboDelegate* delegate);

  bool IsHandledUrl(const GURL& url);

  static std::string GetAcceptLanguagesHeader();

private:
  OperaBrowserContext();
  ~OperaBrowserContext();
};

}  // namespace opera
