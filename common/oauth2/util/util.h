// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_UTIL_UTIL_H_
#define COMMON_OAUTH2_UTIL_UTIL_H_

#include <string>
#include <set>

#include "base/memory/ref_counted.h"

#include "common/oauth2/util/scope_set.h"
#include "common/oauth2/util/token.h"

namespace base {
class DictionaryValue;
}  // namespace base

namespace opera {
namespace oauth2 {
namespace crypt {

std::string OSEncryptInt64(int64_t plainint);
bool OSDecryptInt64(const std::string& ciphertext_base64, int64_t* output);

std::string OSEncrypt(const std::string& plaintext);
std::string OSDecrypt(const std::string& ciphertext_base64);

}  // namespace crypt

std::string LoadStringPref(const base::DictionaryValue* dict,
                           const std::string& path);
int64_t LoadInt64Pref(const base::DictionaryValue* dict,
                      const std::string& path);

// No sorting of these enums, new additions directly before the
// *_LAST_ENUM_VALUE please.
enum SessionEndReason {
  SER_UNKNOWN,
  SER_DISABLE_SYNC_ON_CLIENT_REQUESTED,
  SER_USER_REQUESTED_LOGOUT,
  SER_SYNC_SETUP_CLOSED_DURING_CONFIG,
  SER_USERNAME_CHANGED_DURING_RELOGIN,
  SER_TESTING,

  SER_LAST_ENUM_VALUE
};

enum RequestManagerUrlType { RMUT_UNSET, RMUT_OAUTH1, RMUT_OAUTH2 };

enum SessionStartMethod {
  SSM_UNSET,
  SSM_AUTH_TOKEN,
  SSM_OAUTH1,
  SSM_TESTING,
  SSM_LAST_ENUM_VALUE
};

enum SessionState {
  OA2SS_UNSET,
  OA2SS_INACTIVE,
  OA2SS_STARTING,
  OA2SS_IN_PROGRESS,
  OA2SS_FINISHING,
  OA2SS_AUTH_ERROR,

  OA2SS_LAST_ENUM_VALUE
};

enum OperaAuthError {
  OAE_UNSET,

  OAE_OK,
  OAE_INVALID_CLIENT,
  OAE_INVALID_GRANT,
  OAE_INVALID_REQUEST,
  OAE_INVALID_SCOPE,

  OAE_LAST_ENUM_VALUE
};

/**
 * Used in histogram, only add at the end, no removing.
 */
enum MigrationResult {
  MR_UNSET,

  MR_SUCCESS,
  MR_SUCCESS_WITH_BOUNCE,

  MR_O1_420_NOT_AUTHORIZED_REQUEST,
  MR_O1_421_BAD_REQUEST,
  MR_O1_422_OPERA_USER_NOT_FOUND,
  MR_O1_424_OPERA_TOKEN_NOT_FOUND,
  MR_O1_425_INVALID_OPERA_TOKEN,
  MR_O1_426_COULD_NOT_GENERATE_OPERA_TOKEN,
  MR_O1_428_OPERA_ACCESS_TOKEN_NOT_EXPIRED,
  MR_O1_UNKNOWN,

  MR_O2_UNSET,
  MR_O2_INVALID_CLIENT,
  MR_O2_INVALID_REQUEST,
  MR_O2_INVALID_SCOPE,
  MR_O2_OK,
  MR_O2_INVALID_GRANT,
  MR_O2_UNKNOWN,

  MR_LAST_ENUM_VALUE
};

enum NetworkResponseStatus {
  RS_UNSET,

  // Everything went OK. The request manager will pass the parsed response
  // to the consumer.
  RS_OK,
  // The HTTP status supplied with the response is not valid for the given
  // request. The request manager will retry the request with a backoff
  // delay.
  RS_HTTP_PROBLEM,
  // The response data could not be parsed successfully. The request manager
  // will retry the request with a backoff delay.
  RS_PARSE_PROBLEM,
  // The server explicitly requested throttling with HTTP status 429. The
  // request manager will retry with the server-requested delay.
  RS_THROTTLED,
  // The base accounts URL is not a secure scheme and the base URL for the
  // request cannot be insecure.
  RS_INSECURE_CONNECTION_FORBIDDEN,

  RS_LAST_ENUM_VALUE
};

struct RequestAccessTokenResponse {
  RequestAccessTokenResponse(OperaAuthError auth_error,
                             ScopeSet scopes_requested,
                             scoped_refptr<const AuthToken> access_token);
  RequestAccessTokenResponse(const RequestAccessTokenResponse&);
  ~RequestAccessTokenResponse();

  OperaAuthError auth_error() const;
  ScopeSet scopes_requested() const;
  scoped_refptr<const AuthToken> access_token() const;

 private:
  OperaAuthError auth_error_;
  ScopeSet scopes_requested_;
  ScopeSet scopes_granted_;
  scoped_refptr<const AuthToken> access_token_;
};

struct OAuth1TokenFetchStatus {
  OAuth1TokenFetchStatus();
  ~OAuth1TokenFetchStatus();

  std::unique_ptr<base::DictionaryValue> AsDiagnosticDict() const;

  std::string origin;
  int auth_error;
  std::string error_message;
  base::Time time;
  NetworkResponseStatus response_status;
};

struct OAuth2TokenFetchStatus {
  OAuth2TokenFetchStatus();
  ~OAuth2TokenFetchStatus();

  std::unique_ptr<base::DictionaryValue> AsDiagnosticDict() const;

  std::string origin;
  OperaAuthError auth_error;
  std::string error_message;
  base::Time time;
  NetworkResponseStatus response_status;
};

const char* RequestManagerUrlTypeToString(RequestManagerUrlType type);
const char* NetworkResponseStatusToString(NetworkResponseStatus status);

const char* OperaAuthErrorToString(OperaAuthError type);
OperaAuthError OperaAuthErrorFromInt(int state);
int OperaAuthErrorToInt(OperaAuthError state);

const char* SessionStateToString(SessionState state);
SessionState SessionStateFromInt(int state);
int SessionStateToInt(SessionState state);

const char* SessionStartMethodToString(SessionStartMethod type);
SessionStartMethod SessionStartMethodFromInt(int method);
int SessionStartMethodToInt(SessionStartMethod method);

const char* MigrationResultToString(MigrationResult result);

const char* SessionEndReasonToString(SessionEndReason reason);

}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_UTIL_UTIL_H_
