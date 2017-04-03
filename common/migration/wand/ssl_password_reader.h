// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_WAND_SSL_PASSWORD_READER_H_
#define COMMON_MIGRATION_WAND_SSL_PASSWORD_READER_H_

#include <vector>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"

namespace opera {
namespace common {
namespace migration {

class DataStreamReader;

/** Reads opcert6.dat and extracts system part password and salt. */
class SslPasswordReader : public base::RefCountedThreadSafe<SslPasswordReader> {
 public:
  explicit SslPasswordReader(const base::FilePath& path_to_opcertdat);

  /** Opens the path given in constructor, attempts to extract data.
   * Call this before attempting to GetSystemPartPassword() or
   * GetSystemPartPasswordSalt().
   * @warning Invoke in the File thread.
   * @returns false when there's an error, true it the password and salt were
   * extracted and weren't empty.
   */
  virtual bool Read();
  virtual const std::vector<uint8_t>& GetSystemPartPassword() const;
  virtual const std::vector<uint8_t>& GetSystemPartPasswordSalt() const;

 protected:
  bool ParseSslPasswdRecord(DataStreamReader* reader);
  virtual ~SslPasswordReader();

  friend class base::RefCountedThreadSafe<SslPasswordReader>;

 private:
  base::FilePath path_to_opcertdat_;
  std::vector<uint8_t> system_part_password_;
  std::vector<uint8_t> system_part_password_salt_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_WAND_SSL_PASSWORD_READER_H_
