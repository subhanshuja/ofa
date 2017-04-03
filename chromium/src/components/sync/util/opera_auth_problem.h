// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef SYNC_UTIL_OPERA_AUTH_PROBLEM_H_
#define SYNC_UTIL_OPERA_AUTH_PROBLEM_H_

#if defined(OPERA_SYNC)
#include <string>

#include "base/logging.h"

namespace opera_sync {

// This describes the authorization header verification problem
// as seen by the sync/push services during verification of the
// "Authorization:" header sent by the client with each sync POST
// request and each push XMPP login request.
class OperaAuthProblem {
 public:
  // Source that signals the auth problem.
  enum Source {
    SOURCE_UNKNOWN,
    // The sync service.
    SOURCE_SYNC,
    // The XMPP push service.
    SOURCE_XMPP,
    // Source set during initialization of the
    // current SyncAccount implementation.
    SOURCE_LOGIN,
    // Value used for repeated retry requests in case of
    // a soft failure (e.g. network error).
    SOURCE_RETRY,
    // NOTE: Do not change the order here, no sorting!
    //       New values directly before the boundary!
    //       This is used in statistics.
    // Boundary
    SOURCE_LAST
  };

  // Reason for auth header verification problem, compare SYNC-700, SYNC-702.
  enum Reason {
    REASON_UNSET,
    // 400 - 'Invalid token' - happens when the token state is not
    // "A": authorized.
    // Other possible states are:
    // "E":exchanged; "I":invalidated; "X":expired; "D":marked for deletion.
    REASON_INVALID_TOKEN,
    // 401 - 'Expired token' - the token has been expired.
    REASON_EXPIRED_TOKEN,
    // 402 - 'Invalid consumer application' - the application requesting token
    // verification is not known to Auth server.
    REASON_INVALID_CONSUMER_APP,
    // 403 - 'Token / consumer-key mismatch' - the application requesting token
    // verification is not the one which generated it.
    REASON_TOKEN_KEY_MISMATCH,
    // 404 - 'Bad OAuth request' - most probably the signature is incorrect.
    REASON_BAD_OAUTH_REQUEST,
    // 405 - 'Timestamp incorrect' - timestamp in the request differs from the
    // server time by too much (more than 24 hours).
    REASON_TIMESTAMP_INCORRECT,
    // 406 - 'Invalid timestamp/nonce' - an attempt to use the same nonce string
    // which has been used already (in the last 15 min).
    REASON_INVALID_TIMESTAMP_NONCE,
    // Anything we can't recognize.
    REASON_UNKNOWN_AUTHCODE,
    // NOTE: Do not change the order here, no sorting!
    //       New values directly before the boundary!
    //       This is used in statistics.
    // Boundary
    REASON_LAST
  };

  OperaAuthProblem() {
    source_ = SOURCE_UNKNOWN;
    reason_ = REASON_UNSET;
  }

  OperaAuthProblem(Source source, Reason reason, const std::string& token)
      : source_(source), token_(token), reason_(reason) {}

  std::string ToString() const {
    return "source=" + std::string(SourceToString(source_)) + "; reason=" +
        ReasonToString(reason_) + "; token='" +
        (token_.empty() ? "<EMPTY>" : token_) + "'";
  }

  void set_source(Source source) { source_ = source; }
  void set_token(const std::string& token) { token_ = token; }

  void set_auth_errcode(int auth_errcode) {
    switch (auth_errcode) {
      case 400:
        reason_ = REASON_INVALID_TOKEN;
        break;
      case 401:
        reason_ = REASON_EXPIRED_TOKEN;
        break;
      case 402:
        reason_ = REASON_INVALID_CONSUMER_APP;
        break;
      case 403:
        reason_ = REASON_TOKEN_KEY_MISMATCH;
        break;
      case 404:
        reason_ = REASON_BAD_OAUTH_REQUEST;
        break;
      case 405:
        reason_ = REASON_TIMESTAMP_INCORRECT;
        break;
      case 406:
        reason_ = REASON_INVALID_TIMESTAMP_NONCE;
        break;
      default:
        reason_ = REASON_UNKNOWN_AUTHCODE;
        LOG(ERROR) << "Unknown auth errcode = " << auth_errcode
                   << ", server didn't forward the authentication failure "
                      "reason from the auth server.";
        break;
    }
  }

  Source source() const { return source_; }
  const std::string& token() const { return token_; }
  Reason reason() const { return reason_; }

  bool operator==(const OperaAuthProblem& other) const {
    return source() == other.source() &&
        reason() == other.reason() &&
        token() == other.token();
  }

  static const char* ReasonToString(Reason reason) {
    switch (reason) {
    case REASON_INVALID_TOKEN:
      return "INVALID_TOKEN";
    case REASON_EXPIRED_TOKEN:
      return "EXPIRED_TOKEN";
    case REASON_INVALID_CONSUMER_APP:
      return "INVALID_CONSUMER_APP";
    case REASON_TOKEN_KEY_MISMATCH:
      return "TOKEN_KEY_MISMATCH";
    case REASON_BAD_OAUTH_REQUEST:
      return "BAD_OAUTH_REQUEST";
    case REASON_TIMESTAMP_INCORRECT:
      return "TIMESTAMP_INCORRECT";
    case REASON_INVALID_TIMESTAMP_NONCE:
      return "INVALID_TIMESTAMP_NONCE";
    case REASON_UNKNOWN_AUTHCODE:
      return "UNKNOWN_AUTHCODE";
    case REASON_UNSET:
      return "UNSET";
    default:
      NOTREACHED() << "Unknown reason: " << reason;
      return "<UNKNOWN>";
    }
  }

  static const char* SourceToString(Source source) {
    switch (source) {
      case SOURCE_UNKNOWN:
        return "UNKNOWN";
      case SOURCE_SYNC:
        return "SYNC";
      case SOURCE_XMPP:
        return "XMPP";
      case SOURCE_LOGIN:
        return "LOGIN";
      case SOURCE_RETRY:
        return "RETRY";
      default:
        NOTREACHED() << "Unknown source: " << source;
        return "<UNKNOWN>";
    }
  }

 private:
  Source source_;
  std::string token_;
  Reason reason_;
};

}  // namespace opera_sync

#endif  // OPERA_SYNC
#endif  // SYNC_UTIL_OPERA_AUTH_PROBLEM_H_
