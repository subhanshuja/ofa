// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_SESSION_IMPORTER_H_
#define COMMON_MIGRATION_SESSION_IMPORTER_H_
#include "common/migration/importer.h"
#include "common/migration/sessions/session_import_listener.h"
#include "base/memory/ref_counted.h"
#include "base/compiler_specific.h"

namespace opera {
namespace common {
namespace migration {

class BulkFileReader;
/** Importer for Presto sessions.
 *
 * Session stands for opened windows and tabs. They are stored in
 * autosave.win and potentially other *.win files (if the user saved
 * multiple sessions).
 */
class SessionImporter : public Importer {
 public:
  /**
   * @param reader Should allow finding session files (ie. will usually be set
   * in PROFILE_DIR/sessions.
   * @param listener Will receive the parsed sessions.
   */
  SessionImporter(
      scoped_refptr<BulkFileReader> reader,
      scoped_refptr<SessionImportListener> listener);

  /** Parses the input and notifies a SessionImportListener given in c-tor.
   *
   * @param input Content of autosave.win file.
   * @param listener Will be given to the SessionImportListener after parsing
   * autosave.win or will be called immediately if there's a parsing error.
   */
  void Import(std::unique_ptr<std::istream> input,
              scoped_refptr<MigrationResultListener> listener) override;

 protected:
  ~SessionImporter() override;

 private:
  bool ParseIni(std::istream* input,
                std::vector<ParentWindow>* output) const;
  scoped_refptr<BulkFileReader> reader_;
  scoped_refptr<SessionImportListener> session_listener_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_SESSION_IMPORTER_H_
