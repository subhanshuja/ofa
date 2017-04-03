// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_AUTH_KEEPER_UTIL_H_
#define COMMON_SYNC_SYNC_AUTH_KEEPER_UTIL_H_

#include <string>

#include "base/time/time.h"
#include "components/prefs/pref_service.h"
#include "components/sync/util/opera_auth_problem.h"
#include "components/sync/base/time.h"

#include "common/account/account_util.h"

namespace opera {
namespace sync {

extern const char kNotAvailableString[];

class LastSessionDescription {
 public:
  explicit LastSessionDescription(PrefService* pref_service);
  ~LastSessionDescription();

  void Update(base::TimeDelta duration,
              const std::string& id,
              account_util::LogoutReason logout_reason);

  base::TimeDelta duration() const;
  std::string id() const;
  account_util::LogoutReason logout_reason() const;

  std::string id_string() const;
  std::string logout_reason_string() const;

 private:
  void Load();
  void Save();

  base::TimeDelta duration_;
  std::string id_;
  account_util::LogoutReason logout_reason_;

  PrefService* pref_service_;
};

class TokenRefreshDescription {
 public:
  TokenRefreshDescription();

  void clear();

  void set_expired_time(base::Time expired_time);
  void set_request_time(base::Time request_time);
  void set_scheduled_request_time(base::Time next_request_time);
  void set_request_problem(opera_sync::OperaAuthProblem problem);
  void set_success_time(base::Time success_time);
  void set_error_time(base::Time error_time);
  void set_error_type(account_util::AuthDataUpdaterError error);
  void set_auth_error_type(account_util::AuthOperaComError auth_error);
  void set_request_error_message(std::string message);

  base::Time expired_time() const;
  base::Time request_time() const;
  base::Time scheduled_request_time() const;
  std::string request_problem_string() const;
  std::string request_problem_token_string() const;
  std::string request_problem_source_string() const;
  std::string request_problem_reason_string() const;
  base::Time success_time() const;
  base::Time error_time() const;
  std::string error_type_string() const;
  std::string auth_error_type_string() const;
  std::string request_error_message_string() const;

 private:
  base::Time expired_time_;
  base::Time request_time_;
  base::Time scheduled_request_time_;
  opera_sync::OperaAuthProblem request_problem_;
  base::Time request_success_time_;
  base::Time request_error_time_;
  account_util::AuthDataUpdaterError request_error_type_;
  account_util::AuthOperaComError request_auth_error_type_;
  std::string request_error_message_;
};
}  // namespace sync
}  // namespace opera

#endif  // COMMON_SYNC_SYNC_AUTH_KEEPER_UTIL_H_
