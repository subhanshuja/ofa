// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_PREFS_IMPORTER_H_
#define COMMON_MIGRATION_PREFS_IMPORTER_H_

#include "common/ini_parser/ini_parser.h"
#include "common/migration/importer.h"
#include "base/compiler_specific.h"

namespace opera {
namespace common {
namespace migration {

class PrefsListener : public base::RefCountedThreadSafe<PrefsListener> {
 public:
  /** Called by PrefsImporter when a prefs ini file is parsed.
   *
   * @note This listener will be called on the thread that does file input
   * and parsing. If you plan to do long-running work in response to this call,
   * delegate it to an appropriate thread. The caller assumes this method
   * is fast.
   *
   * @param parser Parser used to parse the ini file, stores all sections,
   * keys and values within. Make a copy if you want to delegate work further,
   * parser will go out of scope when this method returns.
   */
  virtual void OnPrefsArrived(const IniParser& parser) = 0;

 protected:
  friend class base::RefCountedThreadSafe<PrefsListener>;
  virtual ~PrefsListener() {}
};

/** Importer for the prefs file.
 *
 * Presto stored prefs in operaprefs.ini, a simple ini file.
 */
class PrefsImporter : public Importer {
 public:
  /** @param listener Receives the parsed ini file upon completing Import()
   */
  explicit PrefsImporter(scoped_refptr<PrefsListener> listener);

  /** Parses input, which is considered content of operaprefs.ini, and
   * notifies listener.
   * @param input Content of operaprefs.ini.
   */
  void Import(std::unique_ptr<std::istream> input,
              scoped_refptr<MigrationResultListener> listener) override;

 protected:
  ~PrefsImporter() override;

  scoped_refptr<PrefsListener> listener_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_PREFS_IMPORTER_H_
