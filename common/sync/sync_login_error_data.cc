// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_login_error_data.h"

#include "base/json/json_string_value_serializer.h"

namespace opera {

SyncLoginErrorData::SyncLoginErrorData()
    : code(SYNC_LOGIN_UNKNOWN_ERROR) {
}

bool JSONToSyncLoginErrorData(const std::string& json_login_error_data,
                              SyncLoginErrorData* login_error_data,
                              std::string* parse_error) {
  int json_error_code = -1;
  std::string json_error;
  base::DictionaryValue* params_dict = NULL;
  std::unique_ptr<base::Value> params_value(
      JSONStringValueDeserializer(json_login_error_data).Deserialize(
          &json_error_code,
          &json_error));
  if (params_value.get() == nullptr ||
      !params_value->GetAsDictionary(&params_dict)) {
    *parse_error = "Invalid params: " + json_error;
    return false;
  }

  if (params_dict->size() != 2u ||
      !params_dict->GetIntegerWithoutPathExpansion(
          "err_code", &login_error_data->code) ||
      !params_dict->GetStringWithoutPathExpansion(
          "err_msg", &login_error_data->message)) {
    *parse_error = "Invalid params";
    return false;
  }

  return true;
}

}  // namespace opera
