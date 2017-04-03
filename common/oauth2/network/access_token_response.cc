// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/network/access_token_response.h"

#include <memory>

#include "base/json/json_reader.h"
#include "base/values.h"

#include "common/oauth2/network/response_parser.h"
#include "common/oauth2/util/constants.h"

namespace opera {
namespace oauth2 {

AccessTokenResponse::AccessTokenResponse() : auth_error_(OAE_UNSET) {}

AccessTokenResponse::~AccessTokenResponse() {}

NetworkResponseStatus AccessTokenResponse::TryResponse(
    int http_code,
    const std::string& response_data,
    bool expecting_refresh_token) {
  switch (http_code) {
    case 200:
      return ParseForHTTP200(response_data, expecting_refresh_token);
    case 400:
      return ParseForHTTP400(response_data);
    case 401:
      return ParseForHTTP401(response_data);
    default:
      return RS_HTTP_PROBLEM;
  }
}

NetworkResponseStatus AccessTokenResponse::ParseForHTTP200(
    const std::string& response_data,
    bool expecting_refresh_token) {
  std::unique_ptr<base::DictionaryValue> dict(
      base::DictionaryValue::From(base::JSONReader::Read(response_data)));
  if (!dict.get())
    return RS_PARSE_PROBLEM;

  ResponseParser parser;
  // Both user ID and refresh token are mandatory when expecting a refresh
  // token, both are not expected when not expecting the refresh token.
  ResponseParser::Optional refresh_token_optional =
      expecting_refresh_token ? ResponseParser::IS_REQUIRED
                              : ResponseParser::IS_OPTIONAL;
  parser.Expect(kRefreshToken, ResponseParser::STRING, refresh_token_optional);
  parser.Expect(kUserId, ResponseParser::STRING, refresh_token_optional);

  parser.Expect(kAccessToken, ResponseParser::STRING,
                ResponseParser::IS_REQUIRED);
  parser.Expect(kTokenType, ResponseParser::STRING,
                ResponseParser::IS_REQUIRED);
  parser.Expect(kExpiresIn, ResponseParser::INTEGER,
                ResponseParser::IS_REQUIRED);
  parser.Expect(kScope, ResponseParser::STRING, ResponseParser::IS_OPTIONAL);
  if (!parser.Parse(dict.get(), ResponseParser::PARSE_SOFT)) {
    return RS_PARSE_PROBLEM;
  }

  if (parser.GetString(kTokenType) != kBearer) {
    return RS_PARSE_PROBLEM;
  }

  auth_error_ = OAE_OK;
  access_token_ = parser.GetString(kAccessToken);
  // Refresh token is optional (does not come with the refresh token grant)
  if (parser.HasString(kRefreshToken)) {
    refresh_token_ = parser.GetString(kRefreshToken);
  }
  expires_in_ = base::TimeDelta::FromSeconds(parser.GetInteger(kExpiresIn));
  if (parser.HasString(kScope)) {
    scopes_ = ScopeSet::FromEncoded(parser.GetString(kScope));
  }
  if (parser.HasString(kUserId)) {
    user_id_ = parser.GetString(kUserId);
  }

  return RS_OK;
}

NetworkResponseStatus AccessTokenResponse::ParseForHTTP400(
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

  const std::string& error = parser.GetString(kError);

  if (error == kInvalidRequest)
    auth_error_ = OAE_INVALID_REQUEST;
  else
    return RS_PARSE_PROBLEM;

  if (parser.HasString(kErrorDescription)) {
    error_description_ = parser.GetString(kErrorDescription);
  }

  return RS_OK;
}

NetworkResponseStatus AccessTokenResponse::ParseForHTTP401(
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

  const std::string& error = parser.GetString(kError);

  if (error == kInvalidClient)
    auth_error_ = OAE_INVALID_CLIENT;
  else if (error == kInvalidGrant)
    auth_error_ = OAE_INVALID_GRANT;
  else if (error == kInvalidScope)
    auth_error_ = OAE_INVALID_SCOPE;
  else
    return RS_PARSE_PROBLEM;

  if (parser.HasString(kErrorDescription)) {
    error_description_ = parser.GetString(kErrorDescription);
  }

  return RS_OK;
}

}  // namespace oauth2
}  // namespace opera