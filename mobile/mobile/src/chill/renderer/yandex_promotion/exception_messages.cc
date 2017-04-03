// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#include "chill/renderer/yandex_promotion/exception_messages.h"

#include "base/strings/stringprintf.h"

namespace opera {

std::string ExceptionMessages::NotEnoughArguments(unsigned int expected,
                                                  unsigned int provided) {
  return base::StringPrintf("%u argument%s required, but only %u present",
                            expected, (expected > 1 ? "s" : ""), provided);
}

std::string ExceptionMessages::TooManyArguments(unsigned int accepted,
                                                unsigned int provided) {
  return base::StringPrintf("%u argument%s accepted, but %u present", accepted,
                            (accepted > 1 ? "s" : ""), provided);
}

std::string ExceptionMessages::FailedToExecute(const std::string& method,
                                               const std::string& type,
                                               const std::string& detail) {
  return base::StringPrintf("Failed to execute '%s' on '%s'", method.c_str(),
                            type.c_str()) +
         (detail.empty() ? "" : (std::string(": ") + detail));
}

std::string ExceptionMessages::ArgumentNullOrIncorrectType(
    unsigned int argument_index,
    const std::string& expected_type) {
  return base::StringPrintf(
      "Argument number %u is either null, or an invalid %s object",
      argument_index, expected_type.c_str());
}

}  // namespace opera
