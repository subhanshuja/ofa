// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_main_delegate.h
// @final-synchronized

#ifndef CHILL_APP_OPERA_MAIN_DELEGATE_H_
#define CHILL_APP_OPERA_MAIN_DELEGATE_H_

#include <string>

#include "base/compiler_specific.h"
#include "content/public/app/content_main_delegate.h"

#include "chill/common/opera_content_client.h"

namespace content {
class BrowserMainRunner;
}

namespace opera {
class OperaContentBrowserClient;
class OperaContentRendererClient;

class OperaMainDelegate : public content::ContentMainDelegate {
 public:
  OperaMainDelegate();
  virtual ~OperaMainDelegate();

  // ContentMainDelegate implementation:
  bool BasicStartupComplete(int* exit_code) override;
  void PreSandboxStartup() override;
  virtual int RunProcess(
      const std::string& process_type,
      const content::MainFunctionParams& main_function_params) override;
  content::ContentBrowserClient* CreateContentBrowserClient() override;
  virtual content::ContentRendererClient* CreateContentRendererClient()
      override;

  static void InitializeResourceBundle();

 private:
  std::unique_ptr<OperaContentBrowserClient> browser_client_;
  std::unique_ptr<OperaContentRendererClient> renderer_client_;
  OperaContentClient content_client_;

  std::unique_ptr<content::BrowserMainRunner> browser_runner_;

  DISALLOW_COPY_AND_ASSIGN(OperaMainDelegate);
};

}  // namespace opera

#endif  // CHILL_APP_OPERA_MAIN_DELEGATE_H_
