// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_MIGRATION_RESULT_LISTENER_MOCK_H_
#define COMMON_MIGRATION_MIGRATION_RESULT_LISTENER_MOCK_H_
#include "common/migration/migration_result_listener.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace opera {
namespace common {
namespace migration {
namespace ut {

/** Mock for UTs, used widely in Importer tests.
 */
class MigrationResultListenerMock : public MigrationResultListener {
 public:
  MigrationResultListenerMock();
  MOCK_METHOD2(OnImportFinished, void(opera::ImporterType type, bool success));
 private:
  virtual ~MigrationResultListenerMock();
};
}  // ut
}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_MIGRATION_RESULT_LISTENER_MOCK_H_
