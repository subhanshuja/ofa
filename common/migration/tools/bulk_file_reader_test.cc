// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/migration/tools/bulk_file_reader.h"

#include <fstream>

#include "base/files/file_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gmock/include/gmock/gmock.h"

#include "common/migration/tools/test_path_utils.h"

using ::testing::Test;

namespace opera {
namespace common {
namespace migration {

TEST(BulkFileReaderTest, NonExistentFile) {
  base::FilePath base_folder = ut::GetTestDataDir();

  // Make sure there really isn't such a file
  base::FilePath relative_path(FILE_PATH_LITERAL("no_such_file"));
  ASSERT_FALSE(base::PathExists(base_folder.Append(relative_path)));

  scoped_refptr<BulkFileReader> reader = new BulkFileReader(base_folder);
  ASSERT_EQ("", reader->ReadFileContent(relative_path));
}

TEST(BulkFileReaderTest, ExistingFile) {
  base::FilePath base_folder = ut::GetTestDataDir();

  base::FilePath relative_path = base::FilePath().AppendASCII("pstorage").
      AppendASCII("psindex.dat");
  ASSERT_TRUE(base::PathExists(base_folder.Append(relative_path)));

  scoped_refptr<BulkFileReader> reader = new BulkFileReader(base_folder);
  std::ifstream file(base_folder.Append(relative_path).value().c_str());
  std::string expected_content((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
  ASSERT_EQ(expected_content, reader->ReadFileContent(relative_path));
}

}  // namespace migration
}  // namespace common
}  // namespace opera
