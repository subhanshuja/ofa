// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_TOOLS_MOCK_BULK_FILE_READER_H_
#define COMMON_MIGRATION_TOOLS_MOCK_BULK_FILE_READER_H_

#include "common/migration/tools/bulk_file_reader.h"

#include <string>
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace opera {
namespace common {
namespace migration {

class MockBulkFileReader : public BulkFileReader {
 public:
  MockBulkFileReader();
  MOCK_CONST_METHOD1(ReadFileContent,
                     std::string(const base::FilePath& relative_path));
  MOCK_METHOD0(GetNextFile, base::FilePath(void));
 private:
  virtual ~MockBulkFileReader();
};
}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_TOOLS_MOCK_BULK_FILE_READER_H_
