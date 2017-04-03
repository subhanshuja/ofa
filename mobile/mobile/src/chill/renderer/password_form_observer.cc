// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/renderer/password_form_observer.h"

#include <vector>

#include "base/logging.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "components/autofill/core/common/password_form.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "ipc/ipc_message_macros.h"
#include "net/cert/cert_status_flags.h"
#include "third_party/WebKit/public/platform/WebVector.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebFormElement.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebInputElement.h"
#include "third_party/WebKit/public/web/WebNode.h"
#include "third_party/WebKit/public/web/WebUserGestureIndicator.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "url/gurl.h"

#include "chill/common/password_form_messages.h"

using autofill::PasswordForm;
using blink::WebFormElement;
using blink::WebInputElement;

namespace opera {

namespace {

bool IsWebNodeVisibleImpl(const blink::WebNode& node, const int depth) {
  if (depth < 0)
    return false;


  if (!node.isElementNode())
    return false;

  const blink::WebElement element = node.toConst<blink::WebElement>();
  if (element.hasNonEmptyLayoutSize())
    return true;

  for (blink::WebNode child = node.firstChild(); !child.isNull();
       child = child.nextSibling()) {
    if (IsWebNodeVisibleImpl(child, depth - 1))
      return true;
  }
  return false;
}

bool IsWebNodeVisible(const blink::WebNode& node) {
  // The form may have non empty children even when the form's bounding box is
  // empty. Thus we need to look at the form's children.
  const int kNodeSearchDepth = 2;
  return IsWebNodeVisibleImpl(node, kNodeSearchDepth);
}

bool FormsDifferByActionOnly(const autofill::PasswordForm& form1,
                             const autofill::PasswordForm& form2) {
  return (form1.origin == form2.origin &&
          form1.signon_realm == form2.signon_realm &&
          form1.username_element == form2.username_element &&
          form1.username_value == form2.username_value &&
          form1.password_element == form2.password_element &&
          form1.password_value == form2.password_value &&
          form1.action != form2.action);
}

GURL GetFormAction(const WebFormElement& form) {
  // Calculate the canonical action URL
  blink::WebString action = form.action();
  if (action.isNull())
    action = blink::WebString("");  // missing 'action' attribute implies
                                    // current URL
  GURL full_action(form.document().completeURL(action));
  if (!full_action.is_valid())
    return full_action;

  GURL::Replacements rep;
  rep.ClearUsername();
  rep.ClearPassword();
  rep.ClearQuery();
  rep.ClearRef();
  return full_action.ReplaceComponents(rep);
}

}  // namespace

// The next three functions as well as kMaxPasswords are copied from
// password_form_conversion_utils.cc. See WAM-2607.

// Maximum number of password fields we will observe before throwing our
// hands in the air and giving up with a given form.
static const size_t kMaxPasswords = 3;

// Helper to determine which password is the main one, and which is
// an old password (e.g on a "make new password" form), if any.
bool LocateSpecificPasswords(std::vector<WebInputElement> passwords,
                             WebInputElement* password,
                             WebInputElement* old_password) {
  switch (passwords.size()) {
    case 1:
      // Single password, easy.
      *password = passwords[0];
      break;
    case 2:
      if (passwords[0].value() == passwords[1].value()) {
        // Treat two identical passwords as a single password.
        *password = passwords[0];
      } else {
        // Assume first is old password, second is new (no choice but to guess).
        *old_password = passwords[0];
        *password = passwords[1];
      }
      break;
    case 3:
      if (passwords[0].value() == passwords[1].value() &&
          passwords[0].value() == passwords[2].value()) {
        // All three passwords the same? Just treat as one and hope.
        *password = passwords[0];
      } else if (passwords[0].value() == passwords[1].value()) {
        // Two the same and one different -> old password is duplicated one.
        *old_password = passwords[0];
        *password = passwords[2];
      } else if (passwords[1].value() == passwords[2].value()) {
        *old_password = passwords[0];
        *password = passwords[1];
      } else {
        // Three different passwords, or first and last match with middle
        // different. No idea which is which, so no luck.
        return false;
      }
      break;
    default:
      return false;
  }
  return true;
}
// Get information about a login form that encapsulated in the
// PasswordForm struct.
void GetPasswordForm(const WebFormElement& form, PasswordForm* password_form) {
  WebInputElement latest_input_element;
  std::vector<WebInputElement> passwords;
  std::vector<base::string16> other_possible_usernames;

  blink::WebVector<blink::WebFormControlElement> control_elements;
  form.getFormControlElements(control_elements);

  for (size_t i = 0; i < control_elements.size(); ++i) {
    blink::WebFormControlElement control_element = control_elements[i];

    WebInputElement* input_element = toWebInputElement(&control_element);
    if (!input_element || !input_element->isEnabled())
      continue;

    if ((passwords.size() < kMaxPasswords) &&
        input_element->isPasswordField()) {
      // We assume that the username element is the input element before the
      // first password element.
      if (passwords.empty() && !latest_input_element.isNull()) {
        password_form->username_element =
            latest_input_element.nameForAutofill();
        password_form->username_value = latest_input_element.value();
        // Remove the selected username from other_possible_usernames.
        if (!other_possible_usernames.empty() &&
            !latest_input_element.value().isEmpty())
          other_possible_usernames.resize(other_possible_usernames.size() - 1);
      }
      passwords.push_back(*input_element);
    }

    // Various input types such as text, url, email can be a username field.
    if (input_element->isTextField() && !input_element->isPasswordField()) {
      latest_input_element = *input_element;
      // We ignore elements that have no value. Unlike username_element,
      // other_possible_usernames is used only for autofill, not for form
      // identification, and blank autofill entries are not useful.
      if (!input_element->value().isEmpty())
        other_possible_usernames.push_back(input_element->value());
    }
  }

  // Get the document URL
  GURL full_origin(form.document().url());

  // Calculate the canonical action URL
  GURL action = GetFormAction(form);
  if (!action.is_valid())
    return;

  WebInputElement password;
  WebInputElement new_password;
  if (!LocateSpecificPasswords(passwords, &password, &new_password))
    return;

  // We want to keep the path but strip any authentication data, as well as
  // query and ref portions of URL, for the form action and form origin.
  GURL::Replacements rep;
  rep.ClearUsername();
  rep.ClearPassword();
  rep.ClearQuery();
  rep.ClearRef();
  password_form->action = action;
  password_form->origin = full_origin.ReplaceComponents(rep);

  rep.SetPathStr("");
  password_form->signon_realm = full_origin.ReplaceComponents(rep).spec();

  password_form->other_possible_usernames.swap(other_possible_usernames);

  if (!password.isNull()) {
    password_form->password_element = password.nameForAutofill();
    password_form->password_value = password.value();
  }
  if (!new_password.isNull()) {
    password_form->new_password_element = new_password.nameForAutofill();
    password_form->new_password_value = new_password.value();
  }

  password_form->scheme = PasswordForm::SCHEME_HTML;
  password_form->preferred = false;
  password_form->blacklisted_by_user = false;
  password_form->type = PasswordForm::TYPE_MANUAL;
}

static std::unique_ptr<PasswordForm> CreatePasswordForm(
    const WebFormElement& webform) {
  if (webform.isNull())
    return std::unique_ptr<PasswordForm>();

  std::unique_ptr<PasswordForm> password_form(new PasswordForm());
  GetPasswordForm(webform, password_form.get());

  if (!password_form->action.is_valid())
    return std::unique_ptr<PasswordForm>();

  return password_form;
}

PasswordFormObserver::PasswordFormObserver(content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame),
      legacy_(render_frame->GetRenderView(), this) {
  render_frame->GetWebFrame()->setAutofillClient(this);
}

PasswordFormObserver::~PasswordFormObserver() {
}

void PasswordFormObserver::DidFinishDocumentLoad(blink::WebLocalFrame* frame) {
  if (!frame->parent())
    FindAndSendPasswordForms(frame);
}

void PasswordFormObserver::DidFinishLoad(blink::WebLocalFrame* frame) {
  if (!frame->parent()) {
    FindAndSendPasswordForms(frame);
    std::vector<PasswordForm> rendered_password_forms;
    FindValidPasswordForms(*frame, true, &rendered_password_forms);
    Send(new OpPasswordFormMsg_DidFinishLoad(routing_id(),
                                             rendered_password_forms));
  }
}

void PasswordFormObserver::DidStartProvisionalLoad(
    blink::WebLocalFrame* frame) {
  if (!frame->parent()) {
    if (!blink::WebUserGestureIndicator::isProcessingUserGesture() &&
        provisionally_saved_form_.get()) {
      Send(new OpPasswordFormMsg_PasswordFormSubmitted(
          routing_id(), *provisionally_saved_form_, GURL()));
      provisionally_saved_form_.reset();
    }
  }
}

void PasswordFormObserver::OnDestruct() {
  delete this;
}

void PasswordFormObserver::WillSendSubmitEvent(
    const blink::WebFormElement& form) {
  provisionally_saved_form_.reset(CreatePasswordForm(form).release());
}

void PasswordFormObserver::WillSubmitForm(const blink::WebFormElement& form) {
  std::unique_ptr<autofill::PasswordForm> submitted_form =
      CreatePasswordForm(form);
  if (submitted_form) {
    GURL alternative_action_url;
    if (provisionally_saved_form_.get()) {
      if (submitted_form->action == provisionally_saved_form_->action) {
        submitted_form->password_value =
            provisionally_saved_form_->password_value;
      } else if (FormsDifferByActionOnly(*provisionally_saved_form_,
                                         *submitted_form)) {
        submitted_form->password_value =
            provisionally_saved_form_->password_value;
        alternative_action_url = provisionally_saved_form_->action;
      }
    }
    Send(new OpPasswordFormMsg_PasswordFormSubmitted(
        routing_id(), *submitted_form, alternative_action_url));
    provisionally_saved_form_.reset();
  }
}

void PasswordFormObserver::didAssociateFormControlsDynamically() {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();

  // Frame is only processed if it has finished loading, otherwise you can end
  // up with a partially parsed form.
  if (frame && !frame->isLoading())
    FindAndSendPasswordForms(frame);
}

bool PasswordFormObserver::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PasswordFormObserver, message)
    IPC_MESSAGE_HANDLER(OpPasswordFormMsg_FillPasswordForm, OnFillPasswordForm)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void PasswordFormObserver::OnFillPasswordForm(
    const PasswordForm& password_form) {
  WebInputElement username_element;
  WebInputElement password_element;

  if (!FindUsernameAndPasswordElements(password_form, &username_element,
                                       &password_element))
    return;

  FillWebInputElement(&username_element,
                      password_form.username_value);

  FillWebInputElement(&password_element,
                      password_form.password_value);
}

void PasswordFormObserver::FindAndSendPasswordForms(
    blink::WebLocalFrame* frame) {
  std::vector<PasswordForm> parsed_password_forms;
  FindValidPasswordForms(*frame, false, &parsed_password_forms);

  if (parsed_password_forms.empty())
    return;

  Send(new OpPasswordFormMsg_PasswordFormsParsed(routing_id(),
                                                 parsed_password_forms));
}

void PasswordFormObserver::FindValidPasswordForms(
    const blink::WebLocalFrame& frame,
    bool only_rendered,
    std::vector<PasswordForm>* password_forms) {
  blink::WebSecurityOrigin origin = frame.document().getSecurityOrigin();
  if (!origin.canAccessPasswordManager())
    return;

  blink::WebVector<WebFormElement> forms;
  frame.document().forms(forms);

  for (size_t i = 0; i < forms.size(); i++) {
    if (only_rendered && !IsWebNodeVisible(forms[i]))
      continue;

    std::unique_ptr<PasswordForm> password_form(
        CreatePasswordForm(forms[i]));

    // TODO(marejde): password_form.get() is always non-NULL? But looks like all
    // other code that calls CreatePasswordForm checks if it actually was a
    // password form this way. Bug or am I missing something?
    if (!password_form.get())
      continue;

    if (password_form->username_element.empty() ||
        password_form->password_element.empty() ||
        !password_form->action.is_valid())
      continue;

    // TODO(marejde): Chromium skips password manager for SpdyProxy
    // authentication, albeit later on the browser side. We should skip it as
    // well?
    if (base::EndsWith(password_form->signon_realm, "/SpdyProxy",
                       base::CompareCase::SENSITIVE))
      continue;

    password_forms->push_back(*password_form);
  }
}

bool PasswordFormObserver::FindUsernameAndPasswordElements(
    const PasswordForm& password_form,
    WebInputElement* username_element,
    WebInputElement* password_element) {
  GURL::Replacements replacements;
  replacements.ClearUsername();
  replacements.ClearPassword();
  replacements.ClearQuery();
  replacements.ClearRef();

  blink::WebDocument doc = render_frame()->GetWebFrame()->document();
  if (!doc.isHTMLDocument())
    return false;

  GURL full_origin(doc.url());
  if (password_form.origin != full_origin.ReplaceComponents(replacements))
    return false;

  blink::WebVector<WebFormElement> forms;
  doc.forms(forms);

  for (size_t i = 0; i < forms.size(); i++) {
    GURL action = GetFormAction(forms[i]);

    if (password_form.action != action)
      continue;

    if (FindWebInputElement(&forms[i], password_form.username_element,
                            username_element) &&
        FindWebInputElement(&forms[i], password_form.password_element,
                            password_element))
      return true;
  }

  return false;
}

bool PasswordFormObserver::FindWebInputElement(
    WebFormElement* form_element,
    const base::string16& name,
    WebInputElement* input_element) {
  DCHECK(form_element);
  blink::WebVector<blink::WebFormControlElement> elements;
  blink::WebFormControlElement* match = NULL;

  form_element->getFormControlElements(elements);
  for (size_t i = 0; i < elements.size(); i++) {
    if (elements[i].formControlName() == name &&
        elements[i].to<blink::WebElement>().hasHTMLTagName("input")) {
      if (match)
        return false;  // Must be unique.
      match = &elements[i];
    }
  }

  if (!match)
    return false;

  *input_element = match->to<WebInputElement>();

  return true;
}

void PasswordFormObserver::FillWebInputElement(
    blink::WebInputElement* element,
    const base::string16& value) {
  DCHECK(element);
  if (!element->isEnabled() || element->isReadOnly())
    return;

  element->setValue(value);
  element->setAutofilled(true);
}

PasswordFormObserver::LegacyRenderViewObserver::LegacyRenderViewObserver(
    content::RenderView* render_view,
    PasswordFormObserver* parent)
    : content::RenderViewObserver(render_view), parent_(parent) {
}

PasswordFormObserver::LegacyRenderViewObserver::~LegacyRenderViewObserver() {
}

void PasswordFormObserver::LegacyRenderViewObserver::OnDestruct() {
  // No op. This object is owned by PasswordFormObserver.
}

void PasswordFormObserver::LegacyRenderViewObserver::DidFinishLoad(
    blink::WebLocalFrame* frame) {
  parent_->DidFinishLoad(frame);
}

void PasswordFormObserver::LegacyRenderViewObserver::DidFinishDocumentLoad(
    blink::WebLocalFrame* frame) {
  parent_->DidFinishDocumentLoad(frame);
}

void PasswordFormObserver::LegacyRenderViewObserver::DidStartProvisionalLoad(
    blink::WebLocalFrame* frame) {
  parent_->DidStartProvisionalLoad(frame);
}

}  // namespace opera
