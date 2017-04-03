// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2011-2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "migration_master_password.h"
#include "migration_password_encryption.h"
#include <cstring>
#include <memory>

using namespace ::opera::presto_port;

#define OBFUSCATING_PASSWORD \
  { 0x83, 0x7D, 0xFC, 0x0F, 0x8E, 0xB3, 0xE8, 0x69, 0x73, 0xAF, 0xFF }

const char password_verification[] = "Opera SSL Password Verification";

CryptoMasterPasswordHandler::CryptoMasterPasswordHandler() {}

CryptoMasterPasswordHandler::~CryptoMasterPasswordHandler() {}

bool CryptoMasterPasswordHandler::HasMasterPassword() const {
  return !m_system_part_password.empty();
}

bool CryptoMasterPasswordHandler::CheckUserProvidedMasterPassword(
    const std::vector<uint8_t>& master_password) {
  CryptoMasterPasswordEncryption master_password_encryption;

  bool status = false;

  std::vector<uint8_t> seed_vector(
      password_verification,
      password_verification + strlen(password_verification));

  if (!m_system_part_password.empty() && !m_system_part_password_salt.empty()) {
    // We test a decrypt of the check code (m_system_part_password) using the
    // master password
    std::vector<uint8_t> decrypted_part_password;
    status = master_password_encryption.Decrypt(decrypted_part_password,
                                                m_system_part_password,
                                                m_system_part_password_salt,
                                                master_password,
                                                seed_vector);
  }

  return status;
}

void CryptoMasterPasswordHandler::SetMasterPasswordCheckCode(
    const std::vector<uint8_t>& system_part_password_salt,
    const std::vector<uint8_t>& system_part_password) {
  m_system_part_password_salt = system_part_password_salt;
  m_system_part_password = system_part_password;
}

const std::vector<uint8_t>
CryptoMasterPasswordHandler::GetUserProvidedPasswordEncryptionKey(
    const std::vector<uint8_t>& master_password) {

  std::vector<uint8_t> encryption_key;
  // Port from libssl for backward compatibility. The actual master password
  // used for encryption is the user defined password appended with the check
  // code (part password).

  // Create 'complete password' which is used for the actual password for
  // encryption. The complete password is a concatenation of password and the
  // decryption of the password check code 'm_system_part_password'

  CryptoMasterPasswordEncryption master_password_encryption;

  std::vector<uint8_t> decrypted_part_password;

  std::vector<uint8_t> seed_vector(
      password_verification,
      password_verification + strlen(password_verification));
  bool success = master_password_encryption.Decrypt(decrypted_part_password,
                                                    m_system_part_password,
                                                    m_system_part_password_salt,
                                                    master_password,
                                                    seed_vector);

  std::vector<uint8_t> complete_password;

  if (success) {
    complete_password.assign(master_password.begin(), master_password.end());
    complete_password.insert(complete_password.end(),
                             decrypted_part_password.begin(),
                             decrypted_part_password.end());
  }

  return complete_password;
}

const std::vector<uint8_t>
CryptoMasterPasswordHandler::GetObfuscationPasswordEncryptionKey() const {
  const uint8_t obfuscation_password[] = OBFUSCATING_PASSWORD;

  const std::vector<uint8_t> obf(
      obfuscation_password,
      obfuscation_password + sizeof(obfuscation_password));

  return obf;
}
