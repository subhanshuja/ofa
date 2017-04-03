// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/network/oauth1_renew_token_response.h"

#include <memory>
#include <string>

#include "base/json/json_reader.h"
#include "base/values.h"

#include "common/oauth2/network/response_parser.h"
#include "common/oauth2/util/constants.h"

namespace opera {
namespace oauth2 {

OAuth1RenewTokenResponse::OAuth1RenewTokenResponse() : oauth1_error_code_(-1) {}

OAuth1RenewTokenResponse::~OAuth1RenewTokenResponse() {}

NetworkResponseStatus OAuth1RenewTokenResponse::TryResponse(
    int http_code,
    const std::string& response_data) {
  switch (http_code) {
    case 200:
      return ParseForHTTP200(response_data);
    default:
      return RS_HTTP_PROBLEM;
  }
}

int OAuth1RenewTokenResponse::oauth1_error_code() const {
  return oauth1_error_code_;
}

std::string OAuth1RenewTokenResponse::oauth1_error_message() const {
  return oauth1_error_message_;
}

std::string OAuth1RenewTokenResponse::oauth1_auth_token() const {
  return oauth1_auth_token_;
}

std::string OAuth1RenewTokenResponse::oauth1_auth_token_secret() const {
  return oauth1_auth_token_secret_;
}

NetworkResponseStatus OAuth1RenewTokenResponse::ParseForHTTP200(
    const std::string& response_data) {
  std::unique_ptr<base::DictionaryValue> dict(
      base::DictionaryValue::From(base::JSONReader::Read(response_data)));
  if (!dict.get())
    return RS_PARSE_PROBLEM;

  // The OAuth1 server always returns HTTP 200. We need to check if the response
  // JSON can be parsed as a success reposnse *or* an error response.

  ResponseParser ok_response_parser;
  ok_response_parser.Expect(kAuthToken, ResponseParser::STRING,
                            ResponseParser::IS_REQUIRED);
  ok_response_parser.Expect(kAuthTokenSecret, ResponseParser::STRING,
                            ResponseParser::IS_REQUIRED);
  // We need to get at least one of these two for the response to be valid
  ok_response_parser.Expect(kUserNameCamelCase, ResponseParser::STRING,
                            ResponseParser::IS_OPTIONAL);
  ok_response_parser.Expect(kUserEmailCamelCase, ResponseParser::STRING,
                            ResponseParser::IS_OPTIONAL);
  // We don't care much about these.
  ok_response_parser.Expect(kUserIDCamelCase, ResponseParser::STRING,
                            ResponseParser::IS_OPTIONAL);
  ok_response_parser.Expect(kUsedUsernameToLoginCamelCase,
                            ResponseParser::BOOLEAN,
                            ResponseParser::IS_OPTIONAL);

  if (ok_response_parser.Parse(dict.get(), ResponseParser::PARSE_SOFT)) {
    if (!ok_response_parser.HasString(kUserEmailCamelCase) &&
        !ok_response_parser.HasString(kUserNameCamelCase)) {
      return RS_PARSE_PROBLEM;
    }
    oauth1_auth_token_ = ok_response_parser.GetString(kAuthToken);
    oauth1_auth_token_secret_ = ok_response_parser.GetString(kAuthTokenSecret);
    return RS_OK;
  }

  ResponseParser error_response_parser;
  error_response_parser.Expect(kErrCode, ResponseParser::INTEGER,
                               ResponseParser::IS_REQUIRED);
  error_response_parser.Expect(kErrMsg, ResponseParser::STRING,
                               ResponseParser::IS_REQUIRED);
  if (error_response_parser.Parse(dict.get(), ResponseParser::PARSE_SOFT)) {
    oauth1_error_code_ = error_response_parser.GetInteger(kErrCode);
    oauth1_error_message_ = error_response_parser.GetString(kErrMsg);
    return RS_OK;
  }

  return RS_PARSE_PROBLEM;
}

}  // namespace oauth2
}  // namespace opera
