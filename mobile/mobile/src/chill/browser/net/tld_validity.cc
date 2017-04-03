// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/net/tld_validity.h"

namespace opera {
bool TldValidity::HasValidTLD(const GURL& url) {
  const size_t registry_length =
      net::registry_controlled_domains::GetRegistryLength(
          url.host(),
          net::registry_controlled_domains::EXCLUDE_UNKNOWN_REGISTRIES,
          net::registry_controlled_domains::EXCLUDE_PRIVATE_REGISTRIES);
  bool has_known_tld = registry_length != 0;
  return has_known_tld;
}

}  // namespace opera
