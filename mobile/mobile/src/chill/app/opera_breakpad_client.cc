// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/app/shell_breakpad_client.cc

#include "chill/app/opera_breakpad_client.h"

#include "base/path_service.h"
#include "base/files/file_path.h"
#include "content/public/common/content_switches.h"

#include "chill/app/native_interface.h"
#include "chill/common/opera_descriptors.h"
#include "chill/common/opera_switches.h"
#include "chill/common/paths.h"

#include "common/breakpad/breakpad_reporter.h"

namespace opera {

OperaCrashReporterClient::OperaCrashReporterClient() {}

void OperaCrashReporterClient::GetProductNameAndVersion(
    const char** product_name,
    const char** version) {
  *product_name = SOCORRO_PRODUCT_NAME;
  *version = OPR_VERSION;
}

base::FilePath OperaCrashReporterClient::GetReporterLogFilename() {
  return base::FilePath(FILE_PATH_LITERAL("uploads.log"));
}

bool OperaCrashReporterClient::GetCrashDumpLocation(base::FilePath* crash_dir) {
  return PathService::Get(Paths::BREAKPAD_CRASH_DUMP, crash_dir);
}

int OperaCrashReporterClient::GetAndroidMinidumpDescriptor() {
  return kMinidumpDescriptor;
}

bool OperaCrashReporterClient::EnableBreakpadForProcess(
    const std::string& process_type) {
  return process_type == switches::kRendererProcess ||
         process_type == switches::kZygoteProcess ||
         process_type == switches::kGpuProcess;
}

}  // namespace opera
