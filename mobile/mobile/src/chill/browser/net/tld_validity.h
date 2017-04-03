// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_NET_TLD_VALIDITY_H_
#define CHILL_BROWSER_NET_TLD_VALIDITY_H_

#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

namespace opera {

class TldValidity {
 public:
  static bool HasValidTLD(const GURL& url);
};

}  // namespace opera

#endif  // CHILL_BROWSER_NET_TLD_VALIDITY_H_
