// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_MIGRATION_PROFILE_FOLDER_FINDER_H_
#define COMMON_MIGRATION_PROFILE_FOLDER_FINDER_H_

#include "base/files/file_path.h"

namespace opera {
namespace common {
namespace migration {

/** Finds folders that contain required profile files.
 *
 * With some installation types, Presto's profile may have been scattered
 * over several folders. This is the case with the default installation on
 * Windows (some things end up in $AppData/Roaming/..., some in
 * $AppData/Local...). This isn't always the case with other install types, ex.
 * the USB portable install, has the entire profile stored in a single folder,
 * in a different place. This class helps the MigrationAssistant find the input
 * files by providing folders in which they should be.
 */
class ProfileFolderFinder {
 public:
  /** Finds folder in which @a input_file should be found.
   *
   * Thread safety: Callable from any thread but never concurrently.
   * @param input_file path to the input file, relative to a profile folder, ex.
   * "cookies4.dat" or "pstorage/psindex.dat".
   * @param[out] suggested_folder the folder in which @a input_file should be
   * found. Should not be NULL, method does not allocate.
   * @returns whether the Finder knew how to find a folder for @a input_file.
   * If false, @a suggested_folder is left unmodified.
   */
  virtual bool FindFolderFor(const base::FilePath& input_file,
                             base::FilePath* suggested_folder) const = 0;

  virtual ~ProfileFolderFinder() {}
};
}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_PROFILE_FOLDER_FINDER_H_
