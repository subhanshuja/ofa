// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_MIGRATION_WAND_WAND_READER_H_
#define COMMON_MIGRATION_WAND_WAND_READER_H_

#include <vector>

#include "common/migration/tools/binary_reader.h"
#include "common/presto/password_crypto/migration_password_encryption.h"
#include "common/presto/password_crypto/migration_master_password.h"

namespace opera {
namespace common {
namespace migration {

class WandReader : public BinaryReader {
 public:
  WandReader(const std::vector<uint8_t>& encryption_key,
             std::istream* input,
             bool input_is_big_endian = true);

  ~WandReader() override;

  base::string16 ReadEncryptedString16(bool is_password = false);
  bool IsFailed() const override;

 private:
  opera::presto_port::CryptoMasterPasswordEncryption password_decryptor_;
  opera::presto_port::CryptoMasterPasswordHandler encryption_handler_;
  std::vector<uint8_t> encryption_key_;
  // signals decryption error
  bool decryption_failed_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_WAND_WAND_READER_H_
