// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/network/revoke_token_response.h"

#include <memory>

#include "base/json/json_reader.h"
#include "base/memory/ref_counted.h"
#include "base/values.h"

#include "common/oauth2/network/response_parser.h"
#include "common/oauth2/util/constants.h"

namespace opera {
namespace oauth2 {

RevokeTokenResponse::RevokeTokenResponse() : auth_error_(OAE_UNSET) {}

RevokeTokenResponse::~RevokeTokenResponse() {}

NetworkResponseStatus RevokeTokenResponse::TryResponse(
    int http_code,
    const std::string& response_data) {
  switch (http_code) {
    case 200:
      return ParseForHTTP200(response_data);
    case 400:
      return ParseForHTTP400(response_data);
    case 401:
      return ParseForHTTP401(response_data);
    default:
      return RS_HTTP_PROBLEM;
  }
}

NetworkResponseStatus RevokeTokenResponse::ParseForHTTP200(
    const std::string& response_data) {
  if (!response_data.empty())
    return RS_PARSE_PROBLEM;

  return RS_OK;
}

NetworkResponseStatus RevokeTokenResponse::ParseForHTTP400(
    const std::string& response_data) {
  std::unique_ptr<base::DictionaryValue> dict(
      base::DictionaryValue::From(base::JSONReader::Read(response_data)));

  if (!dict.get())
    return RS_PARSE_PROBLEM;

  ResponseParser parser;
  parser.Expect(kError, ResponseParser::STRING, ResponseParser::IS_REQUIRED);
  parser.Expect(kErrorDescription, ResponseParser::STRING,
                ResponseParser::IS_OPTIONAL);
  if (!parser.Parse(dict.get(), ResponseParser::PARSE_SOFT))
    return RS_PARSE_PROBLEM;

  std::string error = parser.GetString(kError);
  if (error == kInvalidRequest)
    auth_error_ = OAE_INVALID_REQUEST;
  else
    return RS_PARSE_PROBLEM;

  if (parser.HasString(kErrorDescription)) {
    error_message_ = parser.GetString(kErrorDescription);
  }
  return RS_OK;
}

NetworkResponseStatus RevokeTokenResponse::ParseForHTTP401(
    const std::string& response_data) {
  std::unique_ptr<base::DictionaryValue> dict(
      base::DictionaryValue::From(base::JSONReader::Read(response_data)));

  if (!dict.get())
    return RS_PARSE_PROBLEM;
  ResponseParser parser;
  parser.Expect(kError, ResponseParser::STRING, ResponseParser::IS_REQUIRED);
  parser.Expect(kErrorDescription, ResponseParser::STRING,
                ResponseParser::IS_OPTIONAL);
  if (!parser.Parse(dict.get(), ResponseParser::PARSE_SOFT))
    return RS_PARSE_PROBLEM;

  std::string error = parser.GetString(kError);
  if (error == kInvalidClient)
    auth_error_ = OAE_INVALID_CLIENT;
  else
    return RS_PARSE_PROBLEM;

  if (parser.HasString(kErrorDescription)) {
    error_message_ = parser.GetString(kErrorDescription);
  }
  return RS_OK;
}

}  // namespace oauth2
}  // namespace opera