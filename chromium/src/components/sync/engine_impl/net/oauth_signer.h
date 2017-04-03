// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef SYNC_ENGINE_NET_OAUTH_SIGNER_H_
#define SYNC_ENGINE_NET_OAUTH_SIGNER_H_

#include <stdint.h>

#include <string>

namespace sync_oauth_signer {

class SignParams {
 public:
  SignParams();
  SignParams(const std::string& realm,
             const std::string& client_key,
             const std::string& client_secret,
             const std::string& token,
             const std::string& secret,
             int64_t time_skew);
  SignParams(const SignParams&);
  ~SignParams();

  std::string GetSignedAuthHeader(const std::string& request_base_url) const;
  bool IsValid() const;
  void UpdateToken(const std::string& new_token,
                   const std::string& new_token_secret);
  bool UpdateTimeSkew(int64_t time_skew);
  void ClearToken();

 private:
  std::string realm_;
  std::string client_key_;
  std::string client_secret_;
  std::string token_;
  std::string secret_;
  int64_t time_skew_;
};

}  // namespace sync_oauth_signer

#endif  // SYNC_ENGINE_NET_OAUTH_SIGNER_H_
