// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_IMPORTER_H_
#define COMMON_MIGRATION_IMPORTER_H_

#include <istream>
#include <memory>

#include "base/memory/ref_counted.h"

#include "common/migration/importer_types.h"
#include "common/migration/migration_result_listener.h"

namespace opera {
namespace common {
namespace migration {

/** Imports some type of user setting or data from Presto Opera into current
 * version.
 * Example: CookieImporter, BookmarksImporter, HistoryImporter etc.
 */
class Importer : public base::RefCountedThreadSafe<Importer> {
 public:
  /** Parse Presto's data from the input stream and apply.
   *
   * @param input Input as read from Presto's profile. Could be a binary
   * datastream, XML, custom text format... Likely an ifstream object.
   * May not be NULL. Importer takes ownership.
   * @param listener Is notified whether import was successfull or not.
   */
  virtual void Import(std::unique_ptr<std::istream> input,
                      scoped_refptr<MigrationResultListener> listener) = 0;

 protected:
  friend class base::RefCountedThreadSafe<Importer>;
  virtual ~Importer() {}
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_IMPORTER_H_
