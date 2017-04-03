// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_MIGRATION_OBML_MIGRATION_RUNNER_H_
#define CHILL_BROWSER_MIGRATION_OBML_MIGRATION_RUNNER_H_

#include <vector>

#include "base/memory/ref_counted.h"
#include "base/callback.h"

#include "chill/browser/migration/obml_font_info.h"
#include "chill/browser/migration/obml_migrator.h"
#include "common/swig_utils/op_arguments.h"

namespace mobile {
class SavedPage;
}  // namespace mobile

namespace opera {

class OBMLMigrationRunner : public base::RefCounted<OBMLMigrationRunner> {
 public:
  // Interface exposed to Java
  static void StartMigration(std::vector<OBMLFontInfo> font_info,
                             base::Callback<void(const OpArguments&)> callback);

  OBMLMigrationRunner(std::vector<OBMLFontInfo> font_info_vector,
                      base::Callback<void(const OpArguments&)> callback);

  virtual ~OBMLMigrationRunner();

 private:
  void Migrate(mobile::SavedPage* saved_page);

  void FinishMigration(mobile::SavedPage* saved_page, bool success);

  OBMLMigrator* migrator_;
  base::Callback<void(const OpArguments&)> callback_;
  bool successful_;

  DISALLOW_COPY_AND_ASSIGN(OBMLMigrationRunner);
};

}  // namespace opera

#endif  // CHILL_BROWSER_MIGRATION_OBML_MIGRATION_RUNNER_H_
