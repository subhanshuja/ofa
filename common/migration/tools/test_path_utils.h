// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_MIGRATION_TOOLS_TEST_PATH_UTILS_H_
#define COMMON_MIGRATION_TOOLS_TEST_PATH_UTILS_H_

#include "base/files/file_path.h"
namespace opera {
namespace common {
namespace migration {
namespace ut {

base::FilePath GetTestDataDir();

}  // namespace ut
}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_TOOLS_TEST_PATH_UTILS_H_
