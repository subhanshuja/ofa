// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/cookie_importer.h"

#include <string>
#include "base/logging.h"

#include "common/migration/cookies/cookie_domain.h"
#include "common/migration/cookies/cookie_import_tags.h"
#include "common/migration/tools/data_stream_reader.h"

namespace opera {
namespace common {
namespace migration {

CookieImporter::CookieImporter(const CookieCallback& on_new_cookie_callback)
    : on_new_cookie_callback_(on_new_cookie_callback) {
}

CookieImporter::~CookieImporter() {
}

void CookieImporter::Import(std::unique_ptr<std::istream> input,
                            scoped_refptr<MigrationResultListener> listener) {
  listener->OnImportFinished(opera::COOKIES,
                             ImportImplementation(input.get()));
}

bool CookieImporter::ImportImplementation(std::istream* input) {
  VLOG(1) << "Importing cookies...";
  DataStreamReader reader(input, true /* big endian */);
  if (!IsSpecOk(&reader)) {
    LOG(ERROR) << "Data stream has invalid spec, aborting import";
    return false;
  }
  uint32_t tag = reader.ReadTag();

  while (!reader.IsFailed() && tag != TAG_COOKIE_DOMAIN_END) {
    if (tag == TAG_COOKIE_DOMAIN_ENTRY) {
      /* New top-level domain found. Parse it and continue. Note that
       * CookieDomain's Parse is recursive and may create more domains as
       * it goes.
       */
      CookieDomain toplevel_domain;
      if (!toplevel_domain.Parse(&reader))
        // Currently, no handling of corrupted input, just return.
        return false;
      toplevel_domains_.push_back(toplevel_domain);
    } else {
      LOG(ERROR) << "Unexpected tag found in cookie file: " << std::hex << tag;
      return false;  // Currently, no handling of corrupted input, just return.
    }

    /* There may be more than one top-level cookie domain (ex. "com" and "gov"),
     * we might find more domain entries, keep looping.
     */
    tag = reader.ReadTag();
  }

  if (reader.IsFailed())
    return false;

  VLOG(1) << "Cookies parsed, storing...";
  for (TopLevelDomainsVector::iterator it = toplevel_domains_.begin();
       it != toplevel_domains_.end();
       ++it)
    (*it).StoreCookies("", on_new_cookie_callback_);

  toplevel_domains_.clear();
  return true;
}

bool CookieImporter::IsSpecOk(DataStreamReader* input) {
  if (input->IsFailed())
    return false;

  const DataStreamReader::Spec& spec = input->GetSpec();
  return (spec.file_version == 4096 &&
          spec.app_version >= 0x2000 &&
          spec.app_version < 0x3000 &&
          spec.tag_length == 1 &&
          spec.len_length == 2);
}

}  // namespace migration
}  // namespace common
}  // namespace opera
