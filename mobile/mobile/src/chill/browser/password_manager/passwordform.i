// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "components/autofill/core/common/password_form.h"
%}

namespace autofill {
struct PasswordForm {
  enum Scheme {
    SCHEME_HTML,
    SCHEME_BASIC,
    SCHEME_DIGEST,
    SCHEME_OTHER
  } scheme;
  std::string signon_realm;
  GURL origin;
  GURL action;
  base::string16 submit_element;
  base::string16 username_element;
  base::string16 username_value;
  base::string16 password_element;
  base::string16 password_value;
};
}

%template(PasswordFormVector) std::vector<autofill::PasswordForm>;
