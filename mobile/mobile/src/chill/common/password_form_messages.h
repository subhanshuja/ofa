// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

// Multiply-included message file, hence no include guard.
// NOLINT(build/header_guard)

#include <vector>

#include "components/autofill/content/common/autofill_param_traits_macros.h"
#include "components/autofill/core/common/password_form.h"
#include "content/public/common/common_param_traits.h"
#include "ipc/ipc_message_macros.h"

#define IPC_MESSAGE_START ShellMsgStart

// Sent from PasswordFormObserver to PasswordFormManager when a document
// has been loaded, at which point we can extract password forms that should
// potentially be auto-filled with login information. All forms are sent, not
// only visible ones, since they might be displayed at a later point.
IPC_MESSAGE_ROUTED1(  // NOLINT(readability/fn_size)
    OpPasswordFormMsg_PasswordFormsParsed,
    std::vector<autofill::PasswordForm> /* parsed_password_forms */)

// Sent from PasswordFormObserver to PasswordFormManager when a document and
// all its resources have been fully loaded. Only password forms that are
// actually rendered are sent so we can determine if the login failed or not. We
// assume that the login fails if a form reappears.
IPC_MESSAGE_ROUTED1(  // NOLINT(readability/fn_size)
    OpPasswordFormMsg_DidFinishLoad,
    std::vector<autofill::PasswordForm> /* rendered_password_forms */)

// Sent from PasswordFormObserver to PasswordFormManager when a form is
// submitted.
// NOLINTNEXTLINE(readability/fn_size)
IPC_MESSAGE_ROUTED2(OpPasswordFormMsg_PasswordFormSubmitted,
                    autofill::PasswordForm /* password_form */,
                    GURL /* alternative_action_url */)

// Sent from PasswordFormManager to PasswordFormObserver to fill a password
// form on the page.
// NOLINTNEXTLINE(readability/fn_size)
IPC_MESSAGE_ROUTED1(OpPasswordFormMsg_FillPasswordForm,
                    autofill::PasswordForm /* password_form */)
