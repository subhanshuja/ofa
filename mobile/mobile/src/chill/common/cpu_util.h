// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_COMMON_OPERA_CPUUTIL_H_
#define CHILL_COMMON_OPERA_CPUUTIL_H_

namespace opera {

extern const char* GetCpuFamilyName();
extern bool IsNeonSupported();

}  // namespace opera

#endif  // CHILL_COMMON_OPERA_CPUUTIL_H_
