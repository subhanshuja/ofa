// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
%}

namespace net {
namespace registry_controlled_domains {

// Same as in registry_controlled_domain.h
enum PrivateRegistryFilter {
  EXCLUDE_PRIVATE_REGISTRIES = 0,
  INCLUDE_PRIVATE_REGISTRIES
};

bool SameDomainOrHost(const GURL& gurl1,
                      const GURL& gurl2,
                      PrivateRegistryFilter filter);

std::string GetDomainAndRegistry(const GURL& gurl,
                                 PrivateRegistryFilter filter);

}
}



