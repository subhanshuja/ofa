// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/ui/chrome_select_file_policy.cc
// @final-synchronized

#ifndef CHILL_BROWSER_OPERA_SELECT_FILE_POLICY_H_
#define CHILL_BROWSER_OPERA_SELECT_FILE_POLICY_H_

#include "base/macros.h"
#include "base/compiler_specific.h"
#include "ui/shell_dialogs/select_file_policy.h"

namespace opera {

class OperaSelectFilePolicy : public ui::SelectFilePolicy {
 public:
  OperaSelectFilePolicy();
  virtual ~OperaSelectFilePolicy();

  // Overridden from ui::SelectFilePolicy:
  bool CanOpenSelectFileDialog() override;
  void SelectFileDenied() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(OperaSelectFilePolicy);
};

}  // namespace opera

#endif  // CHILL_BROWSER_OPERA_SELECT_FILE_POLICY_H_
