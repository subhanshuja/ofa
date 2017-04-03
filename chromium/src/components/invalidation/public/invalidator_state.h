// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INVALIDATION_PUBLIC_INVALIDATOR_STATE_H_
#define COMPONENTS_INVALIDATION_PUBLIC_INVALIDATOR_STATE_H_

#include "components/invalidation/public/invalidation_export.h"

#if defined(OPERA_SYNC)
#include <string>

#include "components/sync/util/opera_auth_problem.h"
#endif  // OPERA_SYNC

namespace syncer {

enum InvalidatorState {
  // Failure states
  // --------------
  // There is an underlying transient problem (e.g., network- or
  // XMPP-related).
  TRANSIENT_INVALIDATION_ERROR,
  DEFAULT_INVALIDATION_ERROR = TRANSIENT_INVALIDATION_ERROR,
  // Our credentials have been rejected.
  INVALIDATION_CREDENTIALS_REJECTED,

  // Called just before shutdown so handlers can unregister themselves.
  INVALIDATOR_SHUTTING_DOWN,

  // Invalidations are fully working.
  INVALIDATIONS_ENABLED
};

#if defined(OPERA_SYNC)
struct OperaInvalidatorState {
  explicit OperaInvalidatorState(InvalidatorState new_state) {
    state = new_state;
  }

  OperaInvalidatorState(InvalidatorState new_state,
                        opera_sync::OperaAuthProblem new_problem) {
    state = new_state;
    problem = new_problem;
  }

  InvalidatorState state;
  opera_sync::OperaAuthProblem problem;
};
#endif  // OPERA_SYNC

INVALIDATION_EXPORT const char* InvalidatorStateToString(
    InvalidatorState state);

#if defined(OPERA_SYNC)
INVALIDATION_EXPORT std::string InvalidatorStateToString(
    OperaInvalidatorState state);
#endif  // OPERA_SYNC

}  // namespace syncer

#endif  // COMPONENTS_INVALIDATION_PUBLIC_INVALIDATOR_STATE_H_
