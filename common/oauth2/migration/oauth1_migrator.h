// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_MIGRATION_OAUTH1_MIGRATOR_H_
#define COMMON_OAUTH2_MIGRATION_OAUTH1_MIGRATOR_H_

#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {

class PersistentSession;

/**
 * OAuth1Migrator manages starting the OAuth2 session depending on the
 * OAuth1 session contents stored in the client local state.
 * The migrator expects an INACTIVE session in the call to StartMigration
 * and it leaves off with an IN_PROGRESS or unchanged INACTIVE session
 * state depending on the migration result.
 * The caller should take precautions about the given session state when
 * performing any tasks around it.
 */
class OAuth1Migrator {
 public:
  virtual ~OAuth1Migrator() {}

  virtual bool IsMigrationPossible() = 0;
  virtual void PrepareMigration(PersistentSession* session) = 0;
  virtual void StartMigration() = 0;
  virtual void EnsureOAuth1SessionIsCleared() = 0;
  virtual MigrationResult GetMigrationResult() = 0;
};
}  // namespace oauth2
}  // namespace opera

#endif  // COMMON_OAUTH2_MIGRATION_OAUTH1_MIGRATOR_H_
