// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2011-2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CRYPTOMASTERPASSWORDENCRYPTION_H_
#define CRYPTOMASTERPASSWORDENCRYPTION_H_

#include <stdint.h>

#include <memory>

#include "migration_master_password.h"

// ATTENTION: this is legacy code meant for migrating presto only,
// it should not be used to encrypt any new data

namespace opera {
namespace presto_port {

namespace crypto {
class CryptoStreamEncryptionCBC;
}

class CryptoMasterPasswordEncryption {
 public:
  CryptoMasterPasswordEncryption();
  virtual ~CryptoMasterPasswordEncryption();

  bool EncryptPassword(std::vector<uint8_t>& encrypted_password_blob,
                       const std::vector<uint8_t>& password_to_encrypt,
                       const std::vector<uint8_t>& encryption_key);
  bool DecryptPassword(std::vector<uint8_t>& decrypted_password,
                       const std::vector<uint8_t>& password_blob,
                       const std::vector<uint8_t>& encryption_key);

 private:
  // Low level
  bool Encrypt(std::vector<uint8_t>& encrypted_data,
               const std::vector<uint8_t>& password_to_encrypt,
               std::vector<uint8_t>& salt,
               int salt_length,
               const std::vector<uint8_t>& encryption_key,
               const std::vector<uint8_t>& plainseed);
  bool Decrypt(std::vector<uint8_t>& decrypted_data,
               const std::vector<uint8_t>& encrypted_data,
               const std::vector<uint8_t>& in_salt,
               const std::vector<uint8_t>& encryption_key,
               const std::vector<uint8_t>& plainseed);

  static void BytesToKey(const std::vector<uint8_t>& in_data,
                         const std::vector<uint8_t>& salt,
                         int hash_iteration_count,
                         std::vector<uint8_t>& out_key,
                         int out_key_length,
                         std::vector<uint8_t>& out_iv,
                         int out_iv_length);

  bool CalculateSalt(const std::vector<uint8_t>& in_data,
                     const std::vector<uint8_t>& password,
                     std::vector<uint8_t>& salt,
                     int salt_length,
                     const std::vector<uint8_t>& plainseed) const;
  bool CalculatedKeyAndIV(const std::vector<uint8_t>& password,
                          const std::vector<uint8_t>& salt) const;

  std::unique_ptr<crypto::CryptoStreamEncryptionCBC> m_des_cbc;

  friend class CryptoMasterPasswordHandler;
};

}  // namespace presto_port
}  // namespace opera

#endif /* CRYPTOMASTERPASSWORDENCRYPTION_H_ */
