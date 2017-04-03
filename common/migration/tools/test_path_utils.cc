// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/migration/tools/test_path_utils.h"

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace opera {
namespace common {
namespace migration {
namespace ut {

base::FilePath GetTestDataDir() {
  base::FilePath path;
  PathService::Get(base::DIR_EXE, &path);
  return path.AppendASCII("test_data").AppendASCII("migration_unittests");
}

}  // namespace ut
}  // namespace migration
}  // namespace common
}  // namespace opera
