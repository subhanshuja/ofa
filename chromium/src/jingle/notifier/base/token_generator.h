// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef JINGLE_NOTIFIER_BASE_TOKEN_GENERATOR_H_
#define JINGLE_NOTIFIER_BASE_TOKEN_GENERATOR_H_

#include <string>

#include "base/callback.h"

namespace opera {

// Called by GenerateTokenCallback with a newly generated token.
// Will be called on the thread which called GenerateTokenCallback.
using TokenReceiver = base::Callback<void(const std::string&)>;

// Defines a callback that calls TokenReceiver with a freshly generated
// oauth_nonce and oauth_timestamp. The double indirection is here to allow
// an asynchronous implementation of the token-generating code.
using GenerateTokenCallback = base::Callback<void(const TokenReceiver&)>;

}  // namespace opera

#endif  // JINGLE_NOTIFIER_BASE_TOKEN_GENERATOR_H_
