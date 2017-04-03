// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_TOOLS_BULK_FILE_READER_H_
#define COMMON_MIGRATION_TOOLS_BULK_FILE_READER_H_

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_enumerator.h"
#include "base/memory/ref_counted.h"

namespace opera {
namespace common {
namespace migration {

/** Reads contents of text files located within some base folder.
 * The files are addressed through a relative path. They should be reasonably
 * small (ie. not dozens of megabytes). Useful for reading in scattered config
 * files and such.
 *
 * @warning Don't use for binary files. Don't use for very large files.
 */
class BulkFileReader : public base::RefCounted<BulkFileReader> {
 public:
  explicit BulkFileReader(const base::FilePath& base_folder);

  /** Read content of file.
   * @param path Path of the text file, absolute or relative to the base folder
   * @return String with file content, empty if file could not be found or
   * opened.
   */
  virtual std::string ReadFileContent(
      const base::FilePath& path) const;

  /** Returns a path to a next file found in the base folder or an empty
   * path if there are no more files.
   * Use this if you need to read multiple files from a folder with names
   * unkown beforehand, ex. saved session files.
   */
  virtual base::FilePath GetNextFile();

 protected:
  friend class base::RefCounted<BulkFileReader>;
  virtual ~BulkFileReader() {}

 private:
  const base::FilePath base_;
  base::FileEnumerator enumerator_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_TOOLS_BULK_FILE_READER_H_
