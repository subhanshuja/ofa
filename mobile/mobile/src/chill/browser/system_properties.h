// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_SYSTEM_PROPERTIES_H_
#define CHILL_BROWSER_SYSTEM_PROPERTIES_H_

#include <string>

namespace opera {

std::string GetSystemProperty(const std::string& key);

}  // namespace opera

#endif  // CHILL_BROWSER_SYSTEM_PROPERTIES_H_
