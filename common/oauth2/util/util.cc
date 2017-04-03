// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/util/util.h"

#include "base/base64.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "components/os_crypt/os_crypt.h"

namespace opera {
namespace oauth2 {
namespace crypt {
std::string OSEncryptInt64(int64_t plainint) {
  const std::string plaintext = base::Int64ToString(plainint);
  return OSEncrypt(plaintext);
}

bool OSDecryptInt64(const std::string& ciphertext_base64, int64_t* output) {
  const std::string plaintext = OSDecrypt(ciphertext_base64);
  return base::StringToInt64(plaintext, output);
}

std::string OSEncrypt(const std::string& plaintext) {
  std::string ciphertext;
  if (!plaintext.empty() && OSCrypt::EncryptString(plaintext, &ciphertext)) {
    std::string ciphertext_base64;
    base::Base64Encode(ciphertext, &ciphertext_base64);
    return ciphertext_base64;
  }

  return std::string();
}

std::string OSDecrypt(const std::string& ciphertext_base64) {
  std::string ciphertext;
  if (!ciphertext_base64.empty() &&
      base::Base64Decode(ciphertext_base64, &ciphertext)) {
    std::string plaintext;
    if (OSCrypt::DecryptString(ciphertext, &plaintext))
      return plaintext;
  }

  return std::string();
}
}  // namespace crypt

std::string LoadStringPref(const base::DictionaryValue* dict,
                           const std::string& path) {
  DCHECK(dict);
  DCHECK(!path.empty());

  std::string plain_string;
  std::string encrypted_string;
  if (dict->GetStringWithoutPathExpansion(path, &encrypted_string)) {
    plain_string = crypt::OSDecrypt(encrypted_string);
  }
  return plain_string;
}

int64_t LoadInt64Pref(const base::DictionaryValue* dict,
                      const std::string& path) {
  DCHECK(dict);
  DCHECK(!path.empty());

  int64_t plain_int = -1;
  std::string encrypted_string;
  if (dict->GetStringWithoutPathExpansion(path, &encrypted_string)) {
    crypt::OSDecryptInt64(encrypted_string, &plain_int);
  }
  return plain_int;
}

RequestAccessTokenResponse::RequestAccessTokenResponse(
    OperaAuthError auth_error,
    ScopeSet scopes_requested,
    scoped_refptr<const AuthToken> access_token)
    : auth_error_(auth_error),
      scopes_requested_(scopes_requested),
      access_token_(access_token) {}

RequestAccessTokenResponse::RequestAccessTokenResponse(
    const RequestAccessTokenResponse&) = default;

RequestAccessTokenResponse::~RequestAccessTokenResponse() {}

OperaAuthError RequestAccessTokenResponse::auth_error() const {
  return auth_error_;
}

ScopeSet RequestAccessTokenResponse::scopes_requested() const {
  return scopes_requested_;
}

scoped_refptr<const AuthToken> RequestAccessTokenResponse::access_token()
    const {
  return access_token_;
}

OAuth1TokenFetchStatus::OAuth1TokenFetchStatus()
    : auth_error(-1), time(base::Time::Now()), response_status(RS_UNSET) {}

OAuth1TokenFetchStatus::~OAuth1TokenFetchStatus() {}

std::unique_ptr<base::DictionaryValue>
OAuth1TokenFetchStatus::AsDiagnosticDict() const {
  std::unique_ptr<base::DictionaryValue> ret(new base::DictionaryValue);

  ret->SetString("origin", origin);
  ret->SetInteger("auth_error", auth_error);
  ret->SetString("error_message", error_message);
  ret->SetDouble("response_time", time.ToJsTime());
  ret->SetString("status", NetworkResponseStatusToString(response_status));
  return ret;
}

OAuth2TokenFetchStatus::OAuth2TokenFetchStatus()
    : auth_error(OAE_UNSET),
      time(base::Time::Now()),
      response_status(RS_UNSET) {}

OAuth2TokenFetchStatus::~OAuth2TokenFetchStatus() {}

std::unique_ptr<base::DictionaryValue>
OAuth2TokenFetchStatus::AsDiagnosticDict() const {
  std::unique_ptr<base::DictionaryValue> ret(new base::DictionaryValue);

  ret->SetString("origin", origin);
  ret->SetString("auth_error", OperaAuthErrorToString(auth_error));
  ret->SetString("error_message", error_message);
  ret->SetDouble("response_time", time.ToJsTime());
  ret->SetString("status", NetworkResponseStatusToString(response_status));
  return ret;
}

const char* OperaAuthErrorToString(OperaAuthError auth_error) {
  switch (auth_error) {
    case OAE_UNSET:
      return "UNSET";
    case OAE_OK:
      return "OK";
    case OAE_INVALID_CLIENT:
      return "INVALID_CLIENT";
    case OAE_INVALID_GRANT:
      return "INVALID_GRANT";
    case OAE_INVALID_REQUEST:
      return "INVALID_REQUEST";
    case OAE_INVALID_SCOPE:
      return "INVALID_SCOPE";
    default:
      NOTREACHED() << auth_error;
      return "<UNKNOWN>";
  }
}

OperaAuthError OperaAuthErrorFromInt(int error) {
  if (error >= OAE_UNSET && error < OAE_LAST_ENUM_VALUE) {
    return static_cast<OperaAuthError>(error);
  }
  VLOG(1) << "Auth error " << error << " is not valid";
  return OAE_UNSET;
}

int OperaAuthErrorToInt(OperaAuthError error) {
  return static_cast<int>(error);
}

const char* SessionStateToString(SessionState state) {
  switch (state) {
    case OA2SS_UNSET:
      return "UNSET";
    case OA2SS_INACTIVE:
      return "INACTIVE";
    case OA2SS_STARTING:
      return "STARTING";
    case OA2SS_IN_PROGRESS:
      return "IN_PROGRESS";
    case OA2SS_FINISHING:
      return "FINISHING";
    case OA2SS_AUTH_ERROR:
      return "AUTH_ERROR";
    case OA2SS_LAST_ENUM_VALUE:
      return "LAST_ENUM_VALUE";
    default:
      NOTREACHED();
      return "<UNKNOWN>";
  }
}

const char* RequestManagerUrlTypeToString(RequestManagerUrlType type) {
  switch (type) {
    case RMUT_OAUTH1:
      return "OAUTH1";
    case RMUT_OAUTH2:
      return "OAUTH2";
    default:
      NOTREACHED();
      return "<UNKNOWN>";
  }
}

const char* NetworkResponseStatusToString(NetworkResponseStatus status) {
  switch (status) {
    case RS_UNSET:
      return "UNSET";
    case RS_OK:
      return "OK";
    case RS_HTTP_PROBLEM:
      return "HTTP_PROBLEM";
    case RS_PARSE_PROBLEM:
      return "PARSE_PROBLEM";
    case RS_THROTTLED:
      return "THROTTLED";
    case RS_INSECURE_CONNECTION_FORBIDDEN:
      return "INSECURE_CONNECTION_FORBIDDEN";
    default:
      NOTREACHED();
      return "<UNKNOWN>";
  }
}

SessionState SessionStateFromInt(int state) {
  if (state >= OA2SS_UNSET && state < OA2SS_LAST_ENUM_VALUE) {
    return static_cast<SessionState>(state);
  }
  VLOG(1) << "Session state " << state << " is not valid";
  return OA2SS_UNSET;
}

int SessionStateToInt(SessionState state) {
  return static_cast<int>(state);
}

const char* SessionStartMethodToString(SessionStartMethod type) {
  switch (type) {
    case SSM_UNSET:
      return "UNSET";
    case SSM_AUTH_TOKEN:
      return "AUTH_TOKEN";
    case SSM_OAUTH1:
      return "OAUTH1";
    case SSM_TESTING:
      return "TESTING";
    default:
      NOTREACHED();
      return "<UNKNOWN>";
  }
}

SessionStartMethod SessionStartMethodFromInt(int method) {
  if (method >= SSM_UNSET && method < SSM_LAST_ENUM_VALUE) {
    return static_cast<SessionStartMethod>(method);
  }
  VLOG(1) << "Session start method " << method << " is not valid";
  return SSM_UNSET;
}

int SessionStartMethodToInt(SessionStartMethod method) {
  return static_cast<int>(method);
}

const char* MigrationResultToString(MigrationResult result) {
  switch (result) {
    case MR_UNSET:
      return "UNSET";
    case MR_SUCCESS:
      return "SUCCESS";
    case MR_SUCCESS_WITH_BOUNCE:
      return "MR_SUCCESS_WITH_BOUNCE";
    case MR_O1_420_NOT_AUTHORIZED_REQUEST:
      return "O1_420_NOT_AUTHORIZED_REQUEST";
    case MR_O1_421_BAD_REQUEST:
      return "O1_421_BAD_REQUEST";
    case MR_O1_422_OPERA_USER_NOT_FOUND:
      return "O1_422_OPERA_USER_NOT_FOUND";
    case MR_O1_424_OPERA_TOKEN_NOT_FOUND:
      return "O1_424_OPERA_TOKEN_NOT_FOUND";
    case MR_O1_425_INVALID_OPERA_TOKEN:
      return "O1_425_INVALID_OPERA_TOKEN";
    case MR_O1_426_COULD_NOT_GENERATE_OPERA_TOKEN:
      return "O1_426_COULD_NOT_GENERATE_OPERA_TOKEN";
    case MR_O1_428_OPERA_ACCESS_TOKEN_NOT_EXPIRED:
      return "O1_428_OPERA_ACCESS_TOKEN_NOT_EXPIRED";
    case MR_O1_UNKNOWN:
      return "O1_UNKNOWN";
    case MR_O2_UNSET:
      return "O2_UNSET";
    case MR_O2_INVALID_CLIENT:
      return "O2_INVALID_CLIENT";
    case MR_O2_INVALID_REQUEST:
      return "O2_INVALID_REQUEST";
    case MR_O2_INVALID_SCOPE:
      return "O2_INVALID_SCOPE";
    case MR_O2_OK:
      return "O2_OK";
    case MR_O2_INVALID_GRANT:
      return "O2_INVALID_GRANT";
    case MR_O2_UNKNOWN:
      return "O2_UNKNOWN";
    default:
      NOTREACHED();
      return "<UNKNOWN>";
  }
}

const char* SessionEndReasonToString(SessionEndReason reason) {
  switch (reason) {
    case SER_UNKNOWN:
      return "UNKNOWN";
    case SER_DISABLE_SYNC_ON_CLIENT_REQUESTED:
      return "DISABLE_SYNC_ON_CLIENT_REQUESTED";
    case SER_USER_REQUESTED_LOGOUT:
      return "SER_USER_REQUESTED_LOGOUT";
    case SER_SYNC_SETUP_CLOSED_DURING_CONFIG:
      return "SYNC_SETUP_CLOSED_DURING_CONFIG";
    case SER_USERNAME_CHANGED_DURING_RELOGIN:
      return "USERNAME_CHANGED_DURING_RELOGIN";
    case SER_TESTING:
      return "TESTING";
    default:
      NOTREACHED();
      return "<UNKNOWN>";
  }
}

}  // namespace oauth2
}  // namespace opera
