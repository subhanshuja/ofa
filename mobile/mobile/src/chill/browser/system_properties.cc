// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/system_properties.h"

#include <sys/system_properties.h>

namespace opera {

std::string GetSystemProperty(const std::string& key) {
  char value[PROP_VALUE_MAX];
  __system_property_get(key.c_str(), value);
  return std::string(value);
}

}  // namespace opera
