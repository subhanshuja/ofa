// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_UTILS_STATFS_UTILS_H_
#define COMMON_UTILS_STATFS_UTILS_H_

#include <stdint.h>
#include <string>

namespace opera {
int64_t getFileSystemId(std::string path);
}  // namespace opera

#endif  // COMMON_UTILS_STATFS_UTILS_H_
