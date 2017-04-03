// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#ifndef CHILL_RENDERER_YANDEX_PROMOTION_EXCEPTION_MESSAGES_H_
#define CHILL_RENDERER_YANDEX_PROMOTION_EXCEPTION_MESSAGES_H_

#include <string>

namespace opera {

// Exception message building is private to Blink core, meaning we have to
// construct our own. These are meant to be equivalent to what is found in
// blink::ExceptionMessages.
class ExceptionMessages {
 public:
  static std::string NotEnoughArguments(unsigned int expected,
                                        unsigned int provided);

  static std::string TooManyArguments(unsigned int accepted,
                                      unsigned int provided);

  static std::string FailedToExecute(const std::string& method,
                                     const std::string& type,
                                     const std::string& detail);

  static std::string ArgumentNullOrIncorrectType(
      unsigned int argument_index,
      const std::string& expected_type);
};

}  // namespace opera

#endif  // CHILL_RENDERER_YANDEX_PROMOTION_EXCEPTION_MESSAGES_H_
