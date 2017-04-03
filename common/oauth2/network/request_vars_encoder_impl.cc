// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/network/request_vars_encoder_impl.h"

#include <iterator>

#include "base/bind.h"
#include "net/base/escape.h"

namespace opera {
namespace oauth2 {

RequestVarsEncoderImpl::RequestVarsEncoderImpl() {}

RequestVarsEncoderImpl::~RequestVarsEncoderImpl() {}

void RequestVarsEncoderImpl::AddVar(const std::string& key,
                                    const std::string& value) {
  DCHECK(!key.empty());
  DCHECK(!value.empty());
  DCHECK_EQ(0u, request_vars_.count(key));
  request_vars_[key] = value;
}

std::string RequestVarsEncoderImpl::GetVar(const std::string& key) {
  DCHECK_EQ(1u, request_vars_.count(key));
  return request_vars_.at(key);
}

bool RequestVarsEncoderImpl::HasVar(const std::string& key) const {
  DCHECK_LE(request_vars_.count(key), 1u);
  return (request_vars_.count(key) == 1u);
}

std::string RequestVarsEncoderImpl::EncodeFormEncoded() const {
  return EncodeWithCallback(
      base::Bind(RequestVarsEncoderImpl::EscapeUrlEncodedDataValueHelper));
}

std::string RequestVarsEncoderImpl::EncodeQueryString() const {
  return EncodeWithCallback(
      base::Bind(RequestVarsEncoderImpl::EscapeQueryParamValueHelper));
}

// static
std::string RequestVarsEncoderImpl::EscapeQueryParamValueHelper(
    const std::string& input) {
  return net::EscapeQueryParamValue(input, false);
}

// static
std::string RequestVarsEncoderImpl::EscapeUrlEncodedDataValueHelper(
    const std::string& input) {
  return net::EscapeUrlEncodedData(input, false);
}

std::string RequestVarsEncoderImpl::EncodeWithCallback(
    base::Callback<std::string(const std::string&)> callback) const {
  DCHECK(!callback.is_null());
  std::string output;
  for (auto it = request_vars_.begin(); it != request_vars_.end(); it++) {
    DCHECK(!it->first.empty());
    DCHECK(!it->second.empty());

    output += callback.Run(it->first) + "=" + callback.Run(it->second);
    if (std::next(it) != request_vars_.end())
      output += "&";
  }
  return output;
}

}  // namespace oauth2
}  // namespace opera
