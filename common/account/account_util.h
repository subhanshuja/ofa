// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_ACCOUNT_ACCOUNT_UTIL_H_
#define COMMON_ACCOUNT_ACCOUNT_UTIL_H_

#include <string>

#include "base/time/time.h"

namespace opera {
namespace account_util {
/* AuthDataUpdaterError describes error related to authorization token/secret
 * fetch/update.
 */
enum AuthDataUpdaterError {
  ADUE_NO_ERROR,
  // Network problem occurred while trying to talk to auth.
  ADUE_NETWORK_ERROR,
  // Auth returned HTTP code other than 200.
  ADUE_HTTP_ERROR,
  // Auth returned a response that could not be parsed.
  ADUE_PARSE_ERROR,
  // Auth returned an error response, spefic error code passed as
  // AuthOperaComError.
  ADUE_AUTH_ERROR,
  // Internal error (old token lost before renewing).
  ADUE_INTERNAL_ERROR
};

/* Auth renewal error codes:
* 420 (NOT_AUTHORIZED_REQUEST) - the request is not authorized, as the
*     service is not allowed to request Opera access token;
* 421 (BAD_REQUEST) - in case wrong request signature for example;
* 422 (OPERA_USER_NOT_FOUND) - the owner of access token, Opera user is
*     not found;
* 424 (OPERA_TOKEN_NOT_FOUND) - access token not found;
* 425 (INVALID_OPERA_TOKEN) - this Opera token has been exchanged for a
*     new one already or has been invalidated by user;
* 426 (COULD_NOT_GENERATE_OPERA_TOKEN) - more an internal error related
*     to inability to issue a new Opera access token.
* 428 (OPERA_ACCESS_TOKEN_IS_NOT_EXPIRED) - the error indicates an attempt
*      to exchange a valid active (not yet expired) token for a new one.
*
* NOTE NOTE NOTE: When adding new enum elements, add them at the end so that
* the numeric valus of the existing elements stay intact. Doing otherwise will
* mess up histogram data.
*/
enum AuthOperaComError {
  // Passed to the failure callback in case the current data updater error
  // is not an auth error.
  AOCE_NO_ERROR,
  // Generic error code passed by the simple oauth token fetcher. The server
  // response is too simple for the client to differentiate between specific
  // failures, moreover, the server response is passed as a string and we
  // don't want to rely on parsing human-readable data.
  AOCE_AUTH_ERROR_SIMPLE_REQUEST,
  // Auth returned an unknown error code during token renewal.
  AOCE_AUTH_ERROR_NOT_RECOGNIZED,
  // Error codes for token renewal.
  AOCE_420_NOT_AUTHORIZED_REQUEST,
  AOCE_421_BAD_REQUEST,
  AOCE_422_OPERA_USER_NOT_FOUND,
  AOCE_424_OPERA_TOKEN_NOT_FOUND,
  AOCE_425_INVALID_OPERA_TOKEN,
  AOCE_426_COULD_NOT_GENERATE_OPERA_TOKEN,
  AOCE_428_OPERA_ACCESS_TOKEN_NOT_EXPIRED,
  // New enum elements go here, no sorting!
  // Boundary, keep this last.
  AOCE_LAST_ENUM_VALUE
};

/* Account logout reason. An element of this enum is passed by the caller of
 * SyncAccount::Logout().
 *
 * No sorting, no adding values in between. Elements of this enum are used
 * in histograms.
 */
enum LogoutReason {
  LR_UNKNOWN,
  // Used in case of a kickout when the token recovery flag is disabled.
  LR_TOKEN_RENEWAL_ERROR_NO_RECOVERY,
  // Used in case the sync server sends a DISABLE_SYNC_ON_CLIENT action.
  // In Opera this is used for account reset.
  LR_DISABLE_SYNC_ON_CLIENT_REQUESTED,
  // User requested explicit logout by clicking the "Logout" button in
  // sync popup.
  LR_POPUP_BUTTON_CLICKED,
  // The sync setup dialog was closed early by user.
  LR_SYNC_SETUP_CLOSED_DURING_CONFIG,
  // Used by the mobile sync manager
  LR_MOBILE_SYNC_MANAGER_LOGOUT,
  // Used in tests only.
  LR_FOR_TEST_ONLY,
  // Boundary.
  LR_LAST_ENUM_VALUE
};

std::string AuthDataUpdaterErrorToString(AuthDataUpdaterError error_code);
std::string AuthOperaComErrorToString(AuthOperaComError error_code);
std::string LogoutReasonToString(LogoutReason logout_reason);
AuthOperaComError AuthOperaComErrorFromInt(int error_code);

enum TDTSPrecision {
  PRECISION_DAYS,
  PRECISION_HOURS,
  PRECISION_MINUTES,
  PRECISION_SECONDS,
  PRECISION_MILLISECONDS,
  PRECISION_MICROSECONDS,
  PRECISION_LAST
};
std::string TimeDeltaToString(base::TimeDelta time_delta,
                              TDTSPrecision precision);

}  // namespace account_util
}  // namespace opera

#endif  // COMMON_ACCOUNT_ACCOUNT_UTIL_H_
