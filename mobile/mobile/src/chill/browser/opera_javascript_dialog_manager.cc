// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chill/browser/opera_javascript_dialog_manager.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/web_contents.h"

#include "chill/common/opera_switches.h"

namespace opera {

OperaJavaScriptDialogManager::OperaJavaScriptDialogManager(
    JavaScriptDialogManagerDelegate* delegate) : delegate_(delegate) {
}

OperaJavaScriptDialogManager::~OperaJavaScriptDialogManager() {
}

void OperaJavaScriptDialogManager::RunJavaScriptDialog(
    content::WebContents* web_contents,
    const GURL& origin_url,
    content::JavaScriptMessageType javascript_message_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    const DialogClosedCallback& callback,
    bool* did_suppress_message) {
  bool suppress = delegate_->GetSuppressMessages();
  const std::string& origin_host = origin_url.host();
  if (!suppress) {
    callback_.SetCallback(callback);
    switch (javascript_message_type) {
      case content::JAVASCRIPT_MESSAGE_TYPE_ALERT: {
        delegate_->RunAlertDialog(origin_host, message_text, &callback_);
        break;
      }
      case content::JAVASCRIPT_MESSAGE_TYPE_CONFIRM: {
        delegate_->RunConfirmDialog(origin_host, message_text, &callback_);
        break;
      }
      case content::JAVASCRIPT_MESSAGE_TYPE_PROMPT: {
        delegate_->RunPromptDialog(origin_host, message_text,
                                   default_prompt_text, &callback_);
        break;
      }
    }
  }

  *did_suppress_message = suppress;
}

void OperaJavaScriptDialogManager::RunBeforeUnloadDialog(
    content::WebContents* web_contents,
    bool is_reload,
    const DialogClosedCallback& callback) {
  callback_.SetCallback(callback);
  delegate_->RunBeforeUnloadDialog(is_reload, &callback_);
}

void OperaJavaScriptDialogManager::CancelActiveAndPendingDialogs(
    content::WebContents* web_contents) {
  delegate_->CancelActiveAndPendingDialogs();
}

void OperaJavaScriptDialogManager::ResetDialogState(
    content::WebContents* web_contents) {
  callback_.SetCallback(base::Bind(OperaJavaScriptDialogManager::StubCallback));
}

bool OperaJavaScriptDialogManager::HandleJavaScriptDialog(
    content::WebContents* web_contents,
    bool accept,
    const base::string16* prompt_override) {
  delegate_->HandleJavaScriptDialog(
      accept, !prompt_override ? base::string16() : *prompt_override);
  return true;
}


/* static */
void OperaJavaScriptDialogManager::StubCallback(bool, const base::string16&) {
}

void OperaDialogClosedCallback::SetCallback(
    const content::JavaScriptDialogManager::DialogClosedCallback& callback) {
  callback_ = callback;
}

void OperaDialogClosedCallback::Run(
    bool success, const base::string16& user_input) {
  callback_.Run(success, user_input);
}

}  // namespace opera
