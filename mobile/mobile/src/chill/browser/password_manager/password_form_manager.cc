// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/password_manager/password_form_manager.h"

#include <vector>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "chrome/browser/renderer_preferences_util.h"
#include "components/autofill/core/common/password_form.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/ssl_status.h"
#include "content/public/common/frame_navigate_params.h"
#include "net/cert/cert_status_flags.h"

#include "chill/browser/chromium_tab.h"
#include "chill/browser/chromium_tab_delegate.h"
#include "chill/browser/password_manager/password_manager_delegate.h"
#include "chill/common/password_form_messages.h"

using autofill::PasswordForm;

namespace {

bool IsFormNonEmpty(const autofill::PasswordForm& form) {
  return (!form.username_value.empty() || !form.password_value.empty());
}

}  // namespace

namespace opera {

PasswordFormManager::PasswordFormManager(ChromiumTab* tab)
    : content::WebContentsObserver(tab->GetWebContents()),
      tab_(tab),
      waiting_for_did_finish_load_(false) {
}

void PasswordFormManager::SetDelegate(PasswordManagerDelegate* delegate) {
  delegate_ = delegate;
}

void PasswordFormManager::DidNavigateMainFrame(
      const content::LoadCommittedDetails& details,
      const content::FrameNavigateParams& params) {
  pending_forms_.clear();
}

bool PasswordFormManager::OnMessageReceived(const IPC::Message& message,
    content::RenderFrameHost* render_frame_host) {
  bool handled = true;
  render_frame_host_ = render_frame_host;
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(PasswordFormManager, message,
                                   render_frame_host)
    IPC_MESSAGE_HANDLER(OpPasswordFormMsg_PasswordFormsParsed,
                        OnPasswordFormsParsed)
    IPC_MESSAGE_HANDLER(OpPasswordFormMsg_DidFinishLoad,
                        OnDidFinishLoad)
    IPC_MESSAGE_HANDLER(OpPasswordFormMsg_PasswordFormSubmitted,
                        OnPasswordFormSubmitted)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void PasswordFormManager::OnPasswordFormsParsed(
    const std::vector<PasswordForm>& parsed_password_forms) {
  bool not_first_parse = !pending_forms_.empty();
  for (std::vector<PasswordForm>::const_iterator i =
      parsed_password_forms.begin(); i != parsed_password_forms.end(); ++i) {
    PasswordForm form = *i;

    // If the form is already on the pending forms list there is no
    // need to add it again. If there are some pending forms and the
    // observed form is filled, but not on the list, it seems safer to
    // skip it rather than autocomplete it. Such a form is probably
    // one of the previously spotted forms that has been modified by a
    // script.
    if ((IsFormNonEmpty(form) && not_first_parse) ||
        FindPendingForm(form) != NULL) {
      continue;
    }

    if (delegate_ && delegate_->FillPasswordForm(&form))
      render_frame_host_->Send(new OpPasswordFormMsg_FillPasswordForm(
          render_frame_host_->GetRoutingID(), form));

    pending_forms_.push_back(form);
  }
}

void PasswordFormManager::OnDidFinishLoad(
    const std::vector<PasswordForm>& rendered_password_forms) {
  if (!waiting_for_did_finish_load_)
    return;

  waiting_for_did_finish_load_ = false;

  if (FormToSaveHasReappeared(rendered_password_forms))
    return;  // Assume login has failed.

  if (delegate_)
    delegate_->PasswordFormLoginSucceeded(form_to_save_);
}

void PasswordFormManager::OnPasswordFormSubmitted(
    const PasswordForm& password_form,
    const GURL& alternative_action_url) {

  if (!password_form.origin.is_valid())
    return;

  PasswordFormSubmitted(password_form, alternative_action_url);

  pending_forms_.clear();
}

bool PasswordFormManager::FormsMatch(const PasswordForm& form1,
                                     const PasswordForm& form2) {
  DCHECK(form1.scheme == PasswordForm::SCHEME_HTML &&
         form2.scheme == PasswordForm::SCHEME_HTML);

  // TODO(marejde): Chromium is a bit more clever when it comes to matching.
  // Can probably improve on it later, but we must at least assure ourselves
  // that there is no security risk with only matching in this way.
  return form1.origin.GetOrigin() == form2.origin.GetOrigin() &&
      form1.action == form2.action &&
      form1.username_element == form2.username_element &&
      form1.password_element == form2.password_element;
}

const PasswordForm* PasswordFormManager::FindPendingForm(
    const PasswordForm& form) {
  for (std::vector<PasswordForm>::const_iterator i = pending_forms_.begin();
      i != pending_forms_.end(); ++i) {
    if (FormsMatch(*i, form))
      return &(*i);
  }
  return NULL;
}

void PasswordFormManager::PasswordFormSubmitted(
    const PasswordForm& form,
    const GURL& alternative_action_url) {
  DCHECK(form.scheme == PasswordForm::SCHEME_HTML);

  if (form.password_value.empty())
    return;

  bool use_alternative_action = false;
  const PasswordForm* pending_form = FindPendingForm(form);
  if (!pending_form && !alternative_action_url.is_empty()) {
    // Not empty alternative action url means that there is a high
    // probability that the submitted form url has been rewritten
    // between the moment user clicked on Submit button and the moment
    // the form was sent. Try whether it is possible to find a form
    // with an alternative action url instead of the one inside the
    // submitted form.
    PasswordForm alternative_form = form;
    alternative_form.action = alternative_action_url;
    pending_form = FindPendingForm(alternative_form);
    use_alternative_action = true;
  }
  if (!pending_form)
    return;

  if (pending_form->username_value == form.username_value &&
      pending_form->password_value == form.password_value)
    return;

  form_to_save_ = *pending_form;
  form_to_save_.action =
      use_alternative_action ? alternative_action_url : form.action;
  form_to_save_.username_value = form.username_value;
  form_to_save_.password_value = form.password_value;

  waiting_for_did_finish_load_ = true;
}

bool PasswordFormManager::FormToSaveHasReappeared(
    const std::vector<PasswordForm>& rendered_forms) {
  for (std::vector<PasswordForm>::const_iterator i = rendered_forms.begin();
      i!= rendered_forms.end(); ++i) {
    if (FormsMatch(form_to_save_, *i))
       return true;
  }
  return false;
}

bool PasswordFormManager::IsWaitingForDidFinishLoad() const {
  return waiting_for_did_finish_load_;
}

}  // namespace opera
