// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/migration/tools/mock_bulk_file_reader.h"


namespace opera {
namespace common {
namespace migration {

MockBulkFileReader::MockBulkFileReader()
  : BulkFileReader(base::FilePath()) {
}

MockBulkFileReader::~MockBulkFileReader() {
}
}  // namespace migration
}  // namespace common
}  // namespace opera
