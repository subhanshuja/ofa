// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_PASSWORD_MANAGER_PASSWORD_FORM_MANAGER_H_
#define CHILL_BROWSER_PASSWORD_MANAGER_PASSWORD_FORM_MANAGER_H_

#include <vector>

#include "base/compiler_specific.h"
#include "components/autofill/core/common/password_form.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class RenderFrameHost;
}

namespace opera {

class PasswordManagerDelegate;
class ChromiumTab;

// Class for handling auto-filling of password forms on the browser side. Asks
// the UI if there are any login credentials available and sends them back to
// the render side if there are. Also stores potential forms and detects when a
// form has been successfully submitted so that login information can be stored.
class PasswordFormManager : public content::WebContentsObserver {
 public:
  explicit PasswordFormManager(ChromiumTab* tab);

  void SetDelegate(PasswordManagerDelegate* delegate);

  virtual void DidNavigateMainFrame(
      const content::LoadCommittedDetails& details,
      const content::FrameNavigateParams& params) override;
  bool OnMessageReceived(
      const IPC::Message& message,
      content::RenderFrameHost* render_frame_host) override;

  bool IsWaitingForDidFinishLoad() const;

 private:
  void OnPasswordFormsParsed(
      const std::vector<autofill::PasswordForm>& parsed_password_forms);
  void OnDidFinishLoad(
      const std::vector<autofill::PasswordForm>& rendered_password_forms);
  void OnPasswordFormSubmitted(const autofill::PasswordForm& password_form,
                               const GURL& alternative_action_url);

  bool FormsMatch(const autofill::PasswordForm& form1,
                  const autofill::PasswordForm& form2);
  const autofill::PasswordForm* FindPendingForm(
      const autofill::PasswordForm& form);
  void PasswordFormSubmitted(const autofill::PasswordForm& form,
                             const GURL& alternative_action_url);
  bool FormToSaveHasReappeared(
      const std::vector<autofill::PasswordForm>& forms);

  ChromiumTab* tab_;
  PasswordManagerDelegate* delegate_;

  std::vector<autofill::PasswordForm> pending_forms_;
  autofill::PasswordForm form_to_save_;
  bool waiting_for_did_finish_load_;

  content::RenderFrameHost* render_frame_host_;

  DISALLOW_COPY_AND_ASSIGN(PasswordFormManager);
};

}  // namespace opera

#endif  // CHILL_BROWSER_PASSWORD_MANAGER_PASSWORD_FORM_MANAGER_H_
