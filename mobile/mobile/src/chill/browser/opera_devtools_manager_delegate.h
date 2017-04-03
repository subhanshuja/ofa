// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
//
// Modified by Opera Software ASA
// NOLINTNEXTLINE(whitespace/line_length)
// @copied-from chromium/src/content/shell/browser/shell_devtools_manager_delegate.h
// @last-synchronized 1ca7ccffe2459dff9e5bdf294256056d1e78ca90

#ifndef CHILL_BROWSER_DEVTOOLS_OPERA_DEVTOOLS_MANAGER_DELEGATE_H_
#define CHILL_BROWSER_DEVTOOLS_OPERA_DEVTOOLS_MANAGER_DELEGATE_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/devtools_manager_delegate.h"

namespace content {
class BrowserContext;
}

namespace opera {

class OperaDevToolsManagerDelegate : public content::DevToolsManagerDelegate {
 public:
  static void StartHttpHandler(content::BrowserContext* browser_context);
  static void StopHttpHandler();
  static int GetHttpHandlerPort();

  explicit OperaDevToolsManagerDelegate(
      content::BrowserContext* browser_context);
  ~OperaDevToolsManagerDelegate() override;

  // DevToolsManagerDelegate implementation.
  scoped_refptr<content::DevToolsAgentHost>
        CreateNewTarget(const GURL& url) override;
  std::string GetDiscoveryPageHTML() override;
  std::string GetFrontendResource(const std::string& path) override;

 private:
  content::BrowserContext* browser_context_;
  DISALLOW_COPY_AND_ASSIGN(OperaDevToolsManagerDelegate);
};

}  // namespace opera

#endif  // CHILL_BROWSER_DEVTOOLS_OPERA_DEVTOOLS_MANAGER_DELEGATE_H_
