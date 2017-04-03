/* -*- Mode: c++; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2016 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#include "common/utils/statfs_utils.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string>

namespace opera {
int64_t getFileSystemId(std::string path) {
  struct stat st;
  if (stat(path.c_str(), &st) != 0) {
    perror("statfs_utils");
    return 0;
  }
  return static_cast<int64_t>(st.st_dev);
}
}  // namespace opera
