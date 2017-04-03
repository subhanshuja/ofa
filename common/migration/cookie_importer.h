// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_COOKIE_IMPORTER_H_
#define COMMON_MIGRATION_COOKIE_IMPORTER_H_
#include <vector>

#include "base/compiler_specific.h"
#include "common/migration/cookies/cookie_callback.h"
#include "common/migration/cookies/cookie_domain.h"
#include "common/migration/importer.h"
#include "net/base/net_export.h"

class Profile;
namespace opera {
namespace common {
namespace migration {

class DataStreamReader;

/** Importer of cookies from Presto Opera.
 *
 * Note that Presto's cookie files stored more than just cookies. They also
 * had cookie-related options, like policies of storing third-party cookies,
 * whether to accept such from specific domains etc. As of writing, these
 * options are not imported, only the actuall cookie lines.
 */
class CookieImporter : public Importer {
 public:
  /** Creates a CookieImporter.
   * @param on_new_cookie_callback will get called upon each new cookie
   * imported as a result of the call to Import().
   */
  explicit CookieImporter(const CookieCallback& on_new_cookie_callback);

  /** Imports cookie data.
   * @param input should be the content of cookies4.dat, may be an open ifstream.
   */
  void Import(std::unique_ptr<std::istream> input,
              scoped_refptr<MigrationResultListener> listener) override;

 protected:
  ~CookieImporter() override;

 private:
  bool ImportImplementation(std::istream* input);
  bool IsSpecOk(DataStreamReader* input);
  CookieCallback on_new_cookie_callback_;
  typedef std::vector<CookieDomain> TopLevelDomainsVector;
  TopLevelDomainsVector toplevel_domains_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_COOKIE_IMPORTER_H_
