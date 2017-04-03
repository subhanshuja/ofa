// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_AUTH_KEEPER_STATUS_H_
#define COMMON_SYNC_SYNC_AUTH_KEEPER_STATUS_H_

#include <array>
#include <map>
#include <memory>
#include <string>

#include "base/time/time.h"
#include "base/values.h"

#define SAKS_XSTR(x) SAKS_STR(x)
#define SAKS_STR(x) #x
#define SAKS_BOOL_PROP(x)                                        \
  void set_##x(bool v) { props_[SAKS_XSTR(x)] = FormatBool(v); } \
  std::string get_##x() const {                                  \
    DCHECK_GT(props_.count(SAKS_XSTR(x)), 0u);                   \
    return props_.at(SAKS_XSTR(x));                              \
  }

#define SAKS_INT_PROP(x)                                       \
  void set_##x(int v) { props_[SAKS_XSTR(x)] = FormatInt(v); } \
  std::string get_##x() const {                                \
    DCHECK_GT(props_.count(SAKS_XSTR(x)), 0u);                 \
    return props_.at(SAKS_XSTR(x));                            \
  }

#define SAKS_STR_PROP(x)                                    \
  void set_##x(std::string v) { props_[SAKS_XSTR(x)] = v; } \
  std::string get_##x() const {                             \
    DCHECK_GT(props_.count(SAKS_XSTR(x)), 0u);              \
    return props_.at(SAKS_XSTR(x));                         \
  }
#define SAKS_TIME_PROP(x)                                              \
  void set_##x(base::Time v) { props_[SAKS_XSTR(x)] = FormatTime(v); } \
  std::string get_##x() const {                                        \
    DCHECK_GT(props_.count(SAKS_XSTR(x)), 0u);                         \
    return props_.at(SAKS_XSTR(x));                                    \
  }

#define SAKS_TIME_DELTA_PROP(x)                \
  void set_##x(base::TimeDelta v) {            \
    props_[SAKS_XSTR(x)] = FormatTimeDelta(v); \
  }                                            \
  std::string get_##x() const {                \
    DCHECK_GT(props_.count(SAKS_XSTR(x)), 0u); \
    return props_.at(SAKS_XSTR(x));            \
  }

namespace opera {
namespace sync {
class SyncAuthKeeperStatus {
 public:
  SyncAuthKeeperStatus();
  SyncAuthKeeperStatus(const SyncAuthKeeperStatus&);
  ~SyncAuthKeeperStatus();
  std::unique_ptr<base::DictionaryValue> AsDict() const;

  // Note that this is not an equality operator per se, it will ignore any
  // keys found in kIgnoredKeys.
  bool IsSameAs(const SyncAuthKeeperStatus& other);
  bool empty() const;

  SAKS_STR_PROP(account__auth_server_url);
  SAKS_TIME_DELTA_PROP(account__current_session_duration);
  SAKS_BOOL_PROP(account__has_auth_data);
  SAKS_BOOL_PROP(account__has_valid_auth_data);
  SAKS_BOOL_PROP(account__is_logged_in);
  SAKS_STR_PROP(account__session_id);
  SAKS_STR_PROP(account__token);
  SAKS_BOOL_PROP(account__used_username_for_login);
  SAKS_STR_PROP(account__user_email);
  SAKS_STR_PROP(account__user_id);
  SAKS_STR_PROP(account__user_name);

  SAKS_TIME_PROP(recovery__next_attempt_time)
  SAKS_STR_PROP(recovery__token_state);
  SAKS_STR_PROP(recovery__token_lost_reason);
  SAKS_STR_PROP(recovery__retry_reason);
  SAKS_BOOL_PROP(recovery__retry_timer_running);
  SAKS_INT_PROP(recovery__periodic_recovery_count);

  SAKS_STR_PROP(refresh__expired_reason);
  SAKS_STR_PROP(refresh__expired_source);
  SAKS_TIME_PROP(refresh__expired_time);
  SAKS_STR_PROP(refresh__expired_token);
  SAKS_TIME_PROP(refresh__scheduled_request_time);
  SAKS_TIME_PROP(refresh__request_time);
  SAKS_STR_PROP(refresh__request_auth_error_type);
  SAKS_STR_PROP(refresh__request_error_message);
  SAKS_TIME_PROP(refresh__request_error_time);
  SAKS_STR_PROP(refresh__request_error_type);
  SAKS_TIME_PROP(refresh__request_success_time);

  SAKS_TIME_DELTA_PROP(last_session__duration);
  SAKS_STR_PROP(last_session__session_id);
  SAKS_STR_PROP(last_session__logout_reason);

  SAKS_STR_PROP(ui__status);

  SAKS_INT_PROP(timestamp);

 private:
  static std::string FormatBool(bool v);
  static std::string FormatInt(int v);
  static std::string FormatTime(base::Time v);
  static std::string FormatTimeDelta(base::TimeDelta v);

  static std::array<std::string, 2> kIgnoredKeys;

  std::map<std::string, std::string> props_;
};

#undef SAKS_XSTR
#undef SAKS_STR
#undef SAKS_BOOL_PROP
#undef SAKS_INT_PROP
#undef SAKS_STR_PROP
#undef SAKS_TIME_PROP
#undef SAKS_TIME_DELTA_PROP
}  // namespace sync
}  // namespace opera
#endif  // COMMON_SYNC_SYNC_AUTH_KEEPER_STATUS_H_
