// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_login_data.h"

#include "base/guid.h"
#include "base/json/json_string_value_serializer.h"

#include "common/sync/sync_config.h"

namespace opera {

namespace {

std::string GetValue(const base::DictionaryValue& dictionary,
                     const std::string& key) {
  std::string value;
  dictionary.GetStringWithoutPathExpansion(key, &value);
  return value;
}

}  // namespace

SyncLoginData::SyncLoginData()
    : used_username_to_login(true),
      session_id(base::GenerateGUID()),
      time_skew(0),
      session_start_time(base::Time::Now()) {
}

SyncLoginData::SyncLoginData(const SyncLoginData&) = default;

SyncLoginData::~SyncLoginData() {
}


bool JSONToSyncLoginData(const std::string& json_login_data,
                         SyncLoginData* login_data,
                         std::string* error) {
  DCHECK(login_data);
  int json_error_code = -1;
  std::string json_error;
  base::DictionaryValue* params_dict = NULL;
  std::unique_ptr<base::Value> params_value(
      JSONStringValueDeserializer(json_login_data).Deserialize(&json_error_code,
                                                               &json_error));
  if (params_value.get() == nullptr ||
      !params_value->GetAsDictionary(&params_dict)) {
    *error = "Invalid params: " + json_error;
    return false;
  }

  login_data->auth_data.token = GetValue(*params_dict, "auth_token");
  login_data->auth_data.token_secret =
      GetValue(*params_dict, "auth_token_secret");

  login_data->user_name = GetValue(*params_dict, "userName");
  login_data->user_id = GetValue(*params_dict, "userID");
  login_data->user_email = GetValue(*params_dict, "userEmail");

  params_dict->GetBooleanWithoutPathExpansion(
      "usedUsernameToLogin", &login_data->used_username_to_login);

  if (SyncConfig::ShouldUseAuthTokenRecovery() &&
      (login_data->auth_data.token.empty() ||
      login_data->auth_data.token_secret.empty())) {
    *error = "Got empty token or secret";
    return false;
  }

  if (login_data->user_name.empty() && login_data->user_email.empty()) {
    *error = "Got empty user name and email";
    return false;
  }

  login_data->password = GetValue(*params_dict, "password");

  return true;
}

bool SyncLoginData::operator==(const SyncLoginData& other) const {
  bool extended_fields_match = true;
  if (SyncConfig::ShouldUseAuthTokenRecovery()) {
    extended_fields_match = (other.session_start_time == session_start_time &&
        other.user_id == user_id);
  }
  return other.auth_data == auth_data &&
         other.used_username_to_login == used_username_to_login &&
         other.user_name == user_name && other.user_email == user_email &&
         other.session_id == session_id && other.time_skew == time_skew &&
         extended_fields_match;
}

}  // namespace opera
