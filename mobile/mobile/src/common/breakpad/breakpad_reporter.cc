// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/breakpad/breakpad_reporter.h"

#include "base/path_service.h"
#include "base/files/file_path.h"
#include "breakpad/src/client/linux/handler/minidump_descriptor.h"
#include "breakpad/src/client/linux/handler/exception_handler.h"

#include "chill/common/paths.h"

namespace opera {

BreakpadReporter::BreakpadReporter() {
  // Registers a ExceptionHandler with a NULL MinidumpCallback, but most
  // importantly with a non-null FilterCallback. As we always opt out of
  // generating an minidump, the descriptor path can be any non-empty string.
  google_breakpad::MinidumpDescriptor descriptor("dummy");
  exception_handler_ = new google_breakpad::ExceptionHandler(
      descriptor,
      &BreakpadReporter::BreakpadCallback,
      NULL,
      this,
      true,
      -1);
}

BreakpadReporter::~BreakpadReporter() {
  delete exception_handler_;
}

void BreakpadReporter::SetCrashDumpsDir(std::string dir) {
  PathService::Override(Paths::BREAKPAD_CRASH_DUMP, base::FilePath(dir));
}

// static
bool BreakpadReporter::BreakpadCallback(void* context) {
  reinterpret_cast<BreakpadReporter*>(context)->UploadDumps();

  // Don't generate a minidump file
  return false;
}

}  // namespace opera
