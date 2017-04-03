// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/opera_javascript_dialog_manager.h"
%}

namespace opera {

%feature("director", assumeoverride=1) JavaScriptDialogManagerDelegate;
%rename("NativeJavaScriptDialogManagerDelegate")
  JavaScriptDialogManagerDelegate;

class OperaDialogClosedCallback {
public:
  void Run(bool success, const base::string16& user_input);
private:
  OperaDialogClosedCallback();
};

class JavaScriptDialogManagerDelegate {
public:
  JavaScriptDialogManagerDelegate();
  virtual ~JavaScriptDialogManagerDelegate();

  virtual void CancelActiveAndPendingDialogs() = 0;

  virtual void RunAlertDialog(
      const std::string& origin_url,
      const base::string16& message_text,
      OperaDialogClosedCallback* callback) = 0;

  virtual void RunConfirmDialog(
      const std::string& origin_url,
      const base::string16& message_text,
      OperaDialogClosedCallback* callback) = 0;

  virtual void RunBeforeUnloadDialog(
      bool is_reload,
      OperaDialogClosedCallback* callback) = 0;

  virtual void RunPromptDialog(
      const std::string& origin_url,
      const base::string16& message_text,
      const base::string16& default_prompt_text,
      OperaDialogClosedCallback* callback) = 0;

  virtual void HandleJavaScriptDialog(
      bool accept, const base::string16& prompt) = 0;

  virtual bool GetSuppressMessages() = 0;
};

}  // namespace opera
