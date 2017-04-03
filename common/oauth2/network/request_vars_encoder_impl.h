// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_NETWORK_REQUEST_VARS_ENCODER_IMPL_H_
#define COMMON_OAUTH2_NETWORK_REQUEST_VARS_ENCODER_IMPL_H_

#include "common/oauth2/network/request_vars_encoder.h"

#include "base/callback.h"

#include <map>
#include <string>

namespace opera {
namespace oauth2 {

class RequestVarsEncoderImpl : public RequestVarsEncoder {
 public:
  RequestVarsEncoderImpl();
  ~RequestVarsEncoderImpl() override;

  void AddVar(const std::string& key, const std::string& value) override;
  std::string GetVar(const std::string& key) override;
  bool HasVar(const std::string& key) const override;
  std::string EncodeFormEncoded() const override;
  std::string EncodeQueryString() const override;

 protected:
  static std::string EscapeQueryParamValueHelper(const std::string& input);
  static std::string EscapeUrlEncodedDataValueHelper(const std::string& input);
  std::string EncodeWithCallback(
      base::Callback<std::string(const std::string&)>) const;

  std::map<std::string, std::string> request_vars_;
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_NETWORK_REQUEST_VARS_ENCODER_IMPL_H_
