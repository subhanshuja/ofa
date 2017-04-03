// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/ui/chrome_select_file_policy.cc
// @final-synchronized

#include "chill/browser/opera_select_file_policy.h"

#include "base/logging.h"

namespace opera {

OperaSelectFilePolicy::OperaSelectFilePolicy() {
}

OperaSelectFilePolicy::~OperaSelectFilePolicy() {}

bool OperaSelectFilePolicy::CanOpenSelectFileDialog() {
  // TODO(WAM, ANDUI): Check if file selection dialogs really are allowed.
  //                   See: chrome/browser/ui/chrome_select_file_policy.cc
  return true;
}

void OperaSelectFilePolicy::SelectFileDenied() {
  // TODO(WAM, ANDUI): Notify user about file selection being denied.
  //                   See: chrome/browser/ui/chrome_select_file_policy.cc
  LOG(WARNING) << "File selection was denied.";
}

}  // namespace opera
