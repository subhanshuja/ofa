// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/fake_invalidation_handler.h"

namespace syncer {

FakeInvalidationHandler::FakeInvalidationHandler()
    : state_(DEFAULT_INVALIDATION_ERROR),
      invalidation_count_(0) {}

FakeInvalidationHandler::~FakeInvalidationHandler() {}

#if defined(OPERA_SYNC)
OperaInvalidatorState FakeInvalidationHandler::GetInvalidatorState() const {
#else
InvalidatorState FakeInvalidationHandler::GetInvalidatorState() const {
#endif  // OPERA_SYNC
  return state_;
}

const ObjectIdInvalidationMap&
FakeInvalidationHandler::GetLastInvalidationMap() const {
  return last_invalidation_map_;
}

int FakeInvalidationHandler::GetInvalidationCount() const {
  return invalidation_count_;
}

#if defined(OPERA_SYNC)
void FakeInvalidationHandler::OnInvalidatorStateChange(
    OperaInvalidatorState state) {
#else
void FakeInvalidationHandler::OnInvalidatorStateChange(InvalidatorState state) {
#endif  // OPERA_SYNC
  state_ = state;
}

void FakeInvalidationHandler::OnIncomingInvalidation(
    const ObjectIdInvalidationMap& invalidation_map) {
  last_invalidation_map_ = invalidation_map;
  ++invalidation_count_;
}

std::string FakeInvalidationHandler::GetOwnerName() const { return "Fake"; }

}  // namespace syncer
