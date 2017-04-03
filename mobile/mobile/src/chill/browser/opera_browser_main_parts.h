// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_browser_main_parts.h
// @final-synchronized

#ifndef CHILL_BROWSER_OPERA_BROWSER_MAIN_PARTS_H_
#define CHILL_BROWSER_OPERA_BROWSER_MAIN_PARTS_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "content/public/browser/browser_main_parts.h"
#include "content/public/common/main_function_params.h"

#include "common/sitepatcher/browser/op_site_patcher.h"

namespace breakpad {
class CrashDumpManager;
}

namespace base {
class Thread;
}

namespace net {
class NetLog;
}

namespace content {
struct MainFunctionParams;
}

namespace opera {

class OperaBrowserContext;

class OperaBrowserMainParts : public content::BrowserMainParts {
 public:
  explicit OperaBrowserMainParts(const content::MainFunctionParams& parameters);
  virtual ~OperaBrowserMainParts();

  // BrowserMainParts overrides.
  void PreEarlyInitialization() override;
  void PreMainMessageLoopStart() override;
  void PostMainMessageLoopStart() override;
  void PreMainMessageLoopRun() override;
  bool MainMessageLoopRun(int* result_code) override;
  void PostMainMessageLoopRun() override;
  int PreCreateThreads() override;

  OperaBrowserContext* browser_context() { return browser_context_.get(); }
  OperaBrowserContext* off_the_record_browser_context() {
    return off_the_record_browser_context_.get();
  }
  net::NetLog* net_log() { return net_log_.get(); }

 private:
  std::unique_ptr<breakpad::CrashDumpManager> crash_dump_manager_;

  std::unique_ptr<net::NetLog> net_log_;
  std::unique_ptr<OperaBrowserContext> browser_context_;
  std::unique_ptr<OperaBrowserContext> off_the_record_browser_context_;

  scoped_refptr<OpSitePatcher> site_patcher_;

  // For running content_browsertests.
  const content::MainFunctionParams parameters_;
  bool run_message_loop_;

  DISALLOW_COPY_AND_ASSIGN(OperaBrowserMainParts);
};

}  // namespace opera

#endif  // CHILL_BROWSER_OPERA_BROWSER_MAIN_PARTS_H_
