// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/app/shell_breakpad_client.h

#ifndef CHILL_APP_OPERA_CRASHREPORTER_CLIENT_H_
#define CHILL_APP_OPERA_CRASHREPORTER_CLIENT_H_

#include <string>

#include "base/macros.h"
#include "base/compiler_specific.h"
#include "components/crash/content/app/crash_reporter_client.h"

namespace opera {

class OperaCrashReporterClient : public crash_reporter::CrashReporterClient {
 public:
  OperaCrashReporterClient();

  // Returns a textual description of the product type and version to include
  // in the crash report.
  virtual void GetProductNameAndVersion(const char** product_name,
                                        const char** version) override;

  base::FilePath GetReporterLogFilename() override;

  // The location where minidump files should be written. Returns true if
  // |crash_dir| was set.
  bool GetCrashDumpLocation(base::FilePath* crash_dir) override;

  // Returns the descriptor key of the android minidump global descriptor.
  int GetAndroidMinidumpDescriptor() override;

  virtual bool EnableBreakpadForProcess(
      const std::string& process_type) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(OperaCrashReporterClient);
};

}  // namespace opera

#endif  // CHILL_APP_OPERA_CRASHREPORTER_CLIENT_H_
