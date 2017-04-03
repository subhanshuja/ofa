// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_RENDERER_PASSWORD_FORM_OBSERVER_H_
#define CHILL_RENDERER_PASSWORD_FORM_OBSERVER_H_

#include <map>
#include <vector>

#include "base/strings/string16.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_view_observer.h"
#include "third_party/WebKit/public/web/WebAutofillClient.h"

namespace content {
class RenderView;
}

namespace autofill {
class PasswordForm;
}  // namespace autofill

namespace blink {
class WebLocalFrame;
class WebInputElement;
class WebFormElement;
}

namespace opera {

// Observer that extracts password forms from a page and sends them to the
// browser side to be auto-filled. It also takes care of filling in the actual
// form on the page if a username and password has been previously saved by the
// user.
class PasswordFormObserver : public content::RenderFrameObserver,
                             public blink::WebAutofillClient {
 public:
  explicit PasswordFormObserver(content::RenderFrame* render_frame);

  // RenderFrameObserver
  void OnDestruct() override;
  void WillSendSubmitEvent(const blink::WebFormElement& form) override;
  void WillSubmitForm(const blink::WebFormElement& form) override;
  bool OnMessageReceived(const IPC::Message& message) override;

  // WebAutofillClient
  void didAssociateFormControlsDynamically() override;
 private:
  class LegacyRenderViewObserver : public content::RenderViewObserver {
   public:
    LegacyRenderViewObserver(content::RenderView* render_view,
                                PasswordFormObserver* parent);
    ~LegacyRenderViewObserver() override;

    // RenderViewObserver:
    void OnDestruct() override;
    void DidFinishLoad(blink::WebLocalFrame* frame) override;
    void DidFinishDocumentLoad(blink::WebLocalFrame* frame) override;
    void DidStartProvisionalLoad(blink::WebLocalFrame* frame) override;

   private:
    PasswordFormObserver* parent_;

    DISALLOW_COPY_AND_ASSIGN(LegacyRenderViewObserver);
  };

  // Called by LegacyRenderViewObserver
  void DidFinishLoad(blink::WebLocalFrame* frame);
  void DidFinishDocumentLoad(blink::WebLocalFrame* frame);
  void DidStartProvisionalLoad(blink::WebLocalFrame* frame);

  // content::RenderFrameObserver::OnDestruct() deletes object.
  virtual ~PasswordFormObserver();

  void OnFillPasswordForm(const autofill::PasswordForm& password_form);

  void FindAndSendPasswordForms(blink::WebLocalFrame* frame);

  void FindValidPasswordForms(
      const blink::WebLocalFrame& frame, bool only_visible,
      std::vector<autofill::PasswordForm>* password_forms);

  bool FindUsernameAndPasswordElements(
      const autofill::PasswordForm& password_form,
      blink::WebInputElement* username_element,
      blink::WebInputElement* password_element);

  bool FindWebInputElement(
      blink::WebFormElement* form_element,
      const base::string16& name,
      blink::WebInputElement* input_element);

  void FillWebInputElement(blink::WebInputElement* element,
                           const base::string16& value);

  // TODO(marejde): Code dealing with provisionally_saved_form_ is copied from
  // PasswordAutofillAgent. Prior to
  // https://chromiumcodereview.appspot.com/19705013 the bugs fixed by having
  // provisionally_saved_forms_ was handled in content. Should look into using
  // the autofill component (see WAM-2607).
  std::unique_ptr<autofill::PasswordForm> provisionally_saved_form_;

  // Passes through |RenderViewObserver| method to |this|.
  LegacyRenderViewObserver legacy_;

  DISALLOW_COPY_AND_ASSIGN(PasswordFormObserver);
};

}  // namespace opera

#endif  // CHILL_RENDERER_PASSWORD_FORM_OBSERVER_H_
