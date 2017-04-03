// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

// Without some knowledge of how breakpad works, the motivation for the
// existence of this class might be difficult to see. The problem it solves is
// that when using the breakpad component in Chromium, we have no control of
// what happens after a minidump file is written. The component takes care of
// wrapping the minidump with some meta-data and moves the file to a designated
// directory, after which the signal handler returns.
//
// Breakpad itself has the ability to register multiple exception handlers which
// will be managed as a stack. When a signal occurs, breakpad goes through the
// stack and lets each handler generate a minidump. Furthermore, exception
// handlers can be created with a FilterCallback which enables opting out of
// generating a minidump. By installing an Exception handler with a
// FilterCallback method /before/ the Chromium component installs its Exception
// handler, the FilterCallback will be invoked /after/ a minidump has been
// created..

#ifndef COMMON_BREAKPAD_BREAKPAD_REPORTER_H_
#define COMMON_BREAKPAD_BREAKPAD_REPORTER_H_

#include <string>

namespace google_breakpad {
// Forward declaration and composition instead of inheritance to avoid pulling
// in multiple include paths from chromium/src/breakpad.
class ExceptionHandler;
}  // namespace google_breakpad

namespace opera {

class BreakpadReporter {
 public:
  BreakpadReporter();
  virtual ~BreakpadReporter();

  virtual void UploadDumps() = 0;

  static void SetCrashDumpsDir(std::string dir);

 private:
  static bool BreakpadCallback(void* context);
  google_breakpad::ExceptionHandler* exception_handler_;
};

}  // namespace opera

#endif  // COMMON_BREAKPAD_BREAKPAD_REPORTER_H_
