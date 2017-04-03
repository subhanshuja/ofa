// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_MIGRATION_RESULT_LISTENER_H_
#define COMMON_MIGRATION_MIGRATION_RESULT_LISTENER_H_

#include <string>
#include "base/callback.h"
#include "base/memory/ref_counted.h"

#include "common/migration/importer_types.h"

namespace opera {
namespace common {
namespace migration {

class MigrationResultListener
    : public base::RefCountedThreadSafe<MigrationResultListener> {
 public:
  /** Called when importing is done.
   * @note May be called from a different thread. Implementation wise, this is
   * triggered by the Importer, when it is sure that everything went fine or
   * not. Since the Importer may use background threads to do work (ex. file IO),
   * a failure/success indication may originate from one.
   * @param item_type Type given when calling MigrationAssistans::Import.
   * @param status true if import was successfull, false otherwise.
   */
  virtual void OnImportFinished(ImporterType item_type, bool status) = 0;

 protected:
  friend class base::RefCountedThreadSafe<MigrationResultListener>;
  virtual ~MigrationResultListener() {}
};

}  // namespace migration
}  // namespace common
}  // namespace opera


#endif  // COMMON_MIGRATION_MIGRATION_RESULT_LISTENER_H_
