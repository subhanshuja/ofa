// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_NETWORK_REQUEST_VARS_ENCODER_H_
#define COMMON_OAUTH2_NETWORK_REQUEST_VARS_ENCODER_H_

#include <string>

namespace opera {
namespace oauth2 {

class RequestVarsEncoder {
 public:
  virtual ~RequestVarsEncoder() {}

  virtual void AddVar(const std::string& key, const std::string& value) = 0;
  virtual std::string GetVar(const std::string& key) = 0;
  virtual bool HasVar(const std::string& key) const = 0;
  virtual std::string EncodeFormEncoded() const = 0;
  virtual std::string EncodeQueryString() const = 0;
};
}  // namespace oauth2
}  // namespace opera

#endif  // COMMON_OAUTH2_NETWORK_REQUEST_VARS_ENCODER_H_
