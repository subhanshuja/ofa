// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/account/account_util.h"

#include <vector>

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"

#if defined(OPERA_DESKTOP)
#include "common/constants/switches.h"
#endif  // OPERA_DESKTOP

namespace opera {
namespace account_util {
std::string AuthDataUpdaterErrorToString(AuthDataUpdaterError error_code) {
  switch (error_code) {
    case ADUE_NO_ERROR:
      return "NO_ERROR";
    case ADUE_NETWORK_ERROR:
      return "NETWORK_ERROR";
    case ADUE_HTTP_ERROR:
      return "HTTP_ERROR";
    case ADUE_PARSE_ERROR:
      return "PARSE_ERROR";
    case ADUE_AUTH_ERROR:
      return "AUTH_ERROR";
    case ADUE_INTERNAL_ERROR:
      return "INTERNAL_ERROR";
    default:
      NOTREACHED();
      return "<UNKNOWN>";
  }
}

std::string AuthOperaComErrorToString(AuthOperaComError auth_error) {
  switch (auth_error) {
    case AOCE_NO_ERROR:
      return "NO_ERROR";
    case AOCE_AUTH_ERROR_SIMPLE_REQUEST:
      return "AUTH_ERROR_SIMPLE_REQUEST";
    case AOCE_420_NOT_AUTHORIZED_REQUEST:
      return "420_NOT_AUTHORIZED_REQUEST";
    case AOCE_421_BAD_REQUEST:
      return "421_BAD_REQUEST";
    case AOCE_422_OPERA_USER_NOT_FOUND:
      return "422_OPERA_TOKEN_NOT_FOUND";
    case AOCE_424_OPERA_TOKEN_NOT_FOUND:
      return "424_OPERA_TOKEN_NOT_FOUND";
    case AOCE_425_INVALID_OPERA_TOKEN:
      return "425_INVALID_OPERA_TOKEN";
    case AOCE_426_COULD_NOT_GENERATE_OPERA_TOKEN:
      return "426_COULD_NOT_GENERATE_OPERA_TOKEN";
    case AOCE_428_OPERA_ACCESS_TOKEN_NOT_EXPIRED:
      return "428_OPERA_ACCESS_TOKEN_NOT_EXPIRED";
    default:
      NOTREACHED();
      return "<UNKNOWN>";
  }
}

std::string LogoutReasonToString(LogoutReason logout_reason) {
  switch (logout_reason) {
    case LR_UNKNOWN:
      return "UNKNOWN";
    case LR_TOKEN_RENEWAL_ERROR_NO_RECOVERY:
      return "TOKEN_RENEWAL_ERROR_NO_RECOVERY";
    case LR_DISABLE_SYNC_ON_CLIENT_REQUESTED:
      return "DISABLE_SYNC_ON_CLIENT_REQUESTED";
    case LR_POPUP_BUTTON_CLICKED:
      return "POPUP_BUTTON_CLICKED";
    case LR_SYNC_SETUP_CLOSED_DURING_CONFIG:
      return "SYNC_SETUP_CLOSED_DURING_CONFIG";
    case LR_FOR_TEST_ONLY:
      return "FOR_TEST_ONLY";
    default:
      NOTREACHED();
      return "<UNKNOWN>";
  }
}

AuthOperaComError AuthOperaComErrorFromInt(int error_code) {
  switch (error_code) {
    case 420:
      return AOCE_420_NOT_AUTHORIZED_REQUEST;
    case 421:
      return AOCE_421_BAD_REQUEST;
    case 422:
      return AOCE_422_OPERA_USER_NOT_FOUND;
    case 424:
      return AOCE_424_OPERA_TOKEN_NOT_FOUND;
    case 425:
      return AOCE_425_INVALID_OPERA_TOKEN;
    case 426:
      return AOCE_426_COULD_NOT_GENERATE_OPERA_TOKEN;
    case 428:
      return AOCE_428_OPERA_ACCESS_TOKEN_NOT_EXPIRED;
    default:
      // Was the error code list extended on the server side?
      NOTREACHED() << "Unknown error code " << error_code;
      return AOCE_AUTH_ERROR_NOT_RECOGNIZED;
  }
}

std::string TimeDeltaToString(base::TimeDelta time_delta,
                              TDTSPrecision precision) {
  const std::string suffix[] = {" d", " h", " min", " s", " ms", " us"};
  static_assert(arraysize(suffix) == (static_cast<int>(PRECISION_LAST)),
                "Check suffix[] against TDTSPrecision.");
  std::vector<std::string> components;
  int days = time_delta.InDays();
  if (days > 0 && precision >= PRECISION_DAYS) {
    time_delta -= base::TimeDelta::FromDays(days);
    components.push_back(base::IntToString(days) + suffix[PRECISION_DAYS]);
  }
  int hours = time_delta.InHours();
  if (hours > 0 && precision >= PRECISION_HOURS) {
    time_delta -= base::TimeDelta::FromHours(hours);
    components.push_back(base::IntToString(hours) + suffix[PRECISION_HOURS]);
  }
  int minutes = time_delta.InMinutes();
  if (minutes > 0 && precision >= PRECISION_MINUTES) {
    time_delta -= base::TimeDelta::FromMinutes(minutes);
    components.push_back(base::IntToString(minutes) +
                         suffix[PRECISION_MINUTES]);
  }
  int64_t seconds = time_delta.InSeconds();
  if (seconds > 0 && precision >= PRECISION_SECONDS) {
    time_delta -= base::TimeDelta::FromSeconds(seconds);
    components.push_back(base::IntToString(seconds) +
                         suffix[PRECISION_SECONDS]);
  }
  int64_t milli = time_delta.InMilliseconds();
  if (milli > 0 && precision >= PRECISION_MILLISECONDS) {
    time_delta -= base::TimeDelta::FromMilliseconds(milli);
    components.push_back(base::IntToString(milli) +
                         suffix[PRECISION_MILLISECONDS]);
  }
  int64_t micro = time_delta.InMicroseconds();
  if (micro > 0 && precision >= PRECISION_MICROSECONDS) {
    time_delta -= base::TimeDelta::FromMicroseconds(micro);
    components.push_back(base::IntToString(micro) +
                         suffix[PRECISION_MICROSECONDS]);
  }

  std::string final_string = "0" + suffix[precision];
  if (components.size() > 0) {
    final_string = base::JoinString(components, " ");
  }
  return final_string;
}

}  // namespace sync_account
}  // namespace opera
