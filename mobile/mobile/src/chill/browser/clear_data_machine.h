// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// NOLINTNEXTLINE(whitespace/line_length)
// @copied-from chromium/src/chrome/browser/browsing_data/browsing_data_remover.cc
// @final-synchronized

#ifndef CHILL_BROWSER_CLEAR_DATA_MACHINE_H_
#define CHILL_BROWSER_CLEAR_DATA_MACHINE_H_

namespace content {
class BrowserContext;
}

namespace opera {

// ClearDataMachine is a state machine with which it is possible to
// asynchronously clear user data such as browser cache, cookies and
// web storage such as LocalStorage, SessionStorage, Web SQL,
// IndexedDB and similar. The intended usage of ClearDataMachine is to
// first create a machine, then set which data should be cleared and
// then lastly calling Clear to run all tasks clearing data.
//
// Example:
//
//   ClearDataMachine* machine = ClearDataMachine::CreateClearDataMachine(
//       browser_context);
//   machine->ScheduleClearData(CLEAR_CACHE);
//   machine->ScheduleClearData(CLEAR_COOKIES)
//   machine->Clear();
//
// This will create and run a machine that will clear cache and
// cookies.
//
// Note that calling ClearDataMachine::Clear will schedule the machine
// for destruction when all tasks are completed so it is not necessary
// to destroy ClearDataMachine unless ClearDataMachine::Clear is never
// called at all.
class ClearDataMachine {
 public:
  // Create a state machine for clearing data
  static ClearDataMachine* CreateClearDataMachine(
      content::BrowserContext* context);

  virtual ~ClearDataMachine() {}

  enum Instruction {
    CLEAR_AUTH_CACHE,
    CLEAR_CACHE,
    CLEAR_COOKIES,
    CLEAR_WEB_STORAGE,
    CLEAR_ALL
  };

  // Instruct the state machine on what data should be cleared.  It is
  // possible to call ScheduleClearData multiple times to specify
  // multiple tasks to perform.
  virtual void ScheduleClearData(Instruction instruction) = 0;

  // Run the state machine. It is not allowed to call Clear multiple
  // times for a given ClearDataMachine.
  //
  // N.B. ClearDataMachine will delete itself as a result of calling
  // ClearDataMachine::Clear
  virtual void Clear() = 0;
};

}  // namespace opera

#endif  // CHILL_BROWSER_CLEAR_DATA_MACHINE_H_
