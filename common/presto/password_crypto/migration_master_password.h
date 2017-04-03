// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CRYPTOMASTERPASSWORDHANDLER_H_
#define CRYPTOMASTERPASSWORDHANDLER_H_
#include <cstdint>
#include <vector>

namespace opera {
namespace presto_port {

class CryptoMasterPasswordHandler {
 public:
  CryptoMasterPasswordHandler();
  ~CryptoMasterPasswordHandler();

  // Return true if master password check codes has been set.
  bool HasMasterPassword() const;

  // Set the check codes loaded from storage
  void SetMasterPasswordCheckCode(
      const std::vector<uint8_t>& system_part_password_salt,
      const std::vector<uint8_t>& system_part_password);

  // Check the master password provided by user against the check codes.
  // return true if password is correct
  bool CheckUserProvidedMasterPassword(
      const std::vector<uint8_t>& user_master_password);

  // Get encryption key from user provided master password
  // This is the encryption key that is used to encrypt/decrypt the passwords
  const std::vector<uint8_t> GetUserProvidedPasswordEncryptionKey(
      const std::vector<uint8_t>& user_master_password);

  // When passwords are not encrypted, they are obfuscated with this key.
  const std::vector<uint8_t> GetObfuscationPasswordEncryptionKey() const;

 private:
  std::vector<uint8_t> m_system_part_password_salt;
  std::vector<uint8_t> m_system_part_password;
};

}  // namespace presto_port
}  // namespace opera

#endif /* CRYPTOMASTERPASSWORDHANDLER_H_ */
