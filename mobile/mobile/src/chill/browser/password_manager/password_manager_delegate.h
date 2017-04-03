// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_PASSWORD_MANAGER_PASSWORD_MANAGER_DELEGATE_H_
#define CHILL_BROWSER_PASSWORD_MANAGER_PASSWORD_MANAGER_DELEGATE_H_

namespace autofill {
class PasswordForm;
}

namespace opera {

class PasswordManagerDelegate {
 public:
  virtual ~PasswordManagerDelegate() {};
  virtual bool FillPasswordForm(autofill::PasswordForm* password_form) = 0;
  virtual void PasswordFormLoginSucceeded(
      const autofill::PasswordForm& password_form) = 0;
};

}  // namespace opera

#endif  // CHILL_BROWSER_PASSWORD_MANAGER_PASSWORD_MANAGER_DELEGATE_H_
