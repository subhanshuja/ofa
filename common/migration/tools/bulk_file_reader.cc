// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/migration/tools/bulk_file_reader.h"

#include <fstream>

namespace opera {
namespace common {
namespace migration {

BulkFileReader::BulkFileReader(const base::FilePath& base_folder)
  : base_(base_folder),
    enumerator_(base_, false, base::FileEnumerator::FILES) {
}

std::string BulkFileReader::ReadFileContent(
    const base::FilePath& path) const {
  base::FilePath p = path.IsAbsolute() ? path : base_.Append(path);
  std::ifstream file(p.value().c_str());
  if (!file.is_open() || !file.good())
    return std::string();

  return std::string((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
}

base::FilePath BulkFileReader::GetNextFile() {
  return enumerator_.Next();
}

}  // namespace migration
}  // namespace common
}  // namespace opera
