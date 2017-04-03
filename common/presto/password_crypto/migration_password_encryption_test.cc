// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gmock/include/gmock/gmock.h"

#include "crypto/migration_encryption_cbc.h"
#include "crypto/migration_crypto_symmetric_algorithm.h"
#include "base/base64.h"
#include "migration_password_encryption.h"

using namespace ::opera::presto_port;

struct PasswordTestData {
  const char* plain;
  const char* encryption_key;
  const char* cipher;
  int cipher_text_length;
};

PasswordTestData password_test_data[] = {
  {"hemmelighet",
  "password",
  "AAAACG4YplmAMIciAAAAEKlZMoHiJQF6835CyuA+Fmc=",
  32},
  {"7 bytes",
  "blagg",
  "AAAACPhWF77HdkFuAAAACDTegjZPP84h",
  24},
  {"8 bytes*",
  "paaefaehfrd",
  "AAAACIwwQrGy8wFBAAAAEArgsHcMxMcY78VAcgPbeYA=",
  32},
  {"9 bytes**",
  "passaefaefea",
  "AAAACC/YXiioSUT3AAAAEDoP5+8E4gzFF40fhuM2XCw=",
  32},
  {"15 bytes*******",
  "paaefaed",
  "AAAACAz5IgwrJBpOAAAAEBgBAlIjT9qVUM3SrtxaHjg=",
  32},
  {"16 bytes********",
  "rd",
  "AAAACLmhT01uj+ZzAAAAGNbvURK3IeqqPVV5BdEIfTe54RIDRsA8ug==",
  40},
  {"17 bytes*********",
  "paefaef",
  "AAAACPEu2Na/1nykAAAAGA9dAZrC6MGZ0z/4aXhcWh8ehLUs6uBBsA==",
  40},
  {"",
  "",
  "AAAACMyXuY8htD74AAAACNs1Fv3+LMVp",
  24},
  {"",
  "password",
  "AAAACCHJ3ggS1I+uAAAACG/DfvnsYneQ",
  24},
  {0, 0, 0, 0}};

void EncryptBlob(CryptoMasterPasswordEncryption& encryptor,
                           const std::vector<uint8_t>& plain,
                           const std::vector<uint8_t>& encryption_key,
                           const char* verified_cipher_text_base64,
                           int cipher_text_length,
                           std::vector<uint8_t>& ciphertext /*out*/) {
  std::vector<uint8_t> verified_cipher_text_base64_vector(
      verified_cipher_text_base64,
      verified_cipher_text_base64 + strlen(verified_cipher_text_base64));

  std::string decoded_binary_output;

  base::Base64Decode(base::StringPiece(verified_cipher_text_base64),
                     &decoded_binary_output);
  std::vector<uint8_t> verified_cipher_text(decoded_binary_output.begin(),
                                          decoded_binary_output.end());

  EXPECT_EQ(cipher_text_length, (int)decoded_binary_output.length());

  EXPECT_TRUE(
      encryptor.EncryptPassword(ciphertext, plain, encryption_key));
  EXPECT_EQ(verified_cipher_text, ciphertext);
}

void DecryptBlob(CryptoMasterPasswordEncryption& encryptor,
                 const std::vector<uint8_t>& plain,
                 const std::vector<uint8_t>& encryption_key,
                 const std::vector<uint8_t>& ciphertext) {

  std::vector<uint8_t> decrypted_master_password;
  std::vector<uint8_t> cipher_text_vector(ciphertext.begin(), ciphertext.end());

  EXPECT_TRUE(encryptor.DecryptPassword(
      decrypted_master_password, ciphertext, encryption_key));

  EXPECT_EQ(plain, decrypted_master_password);
}

TEST(PasswordTest, PasswordTest) {

  int i = 0;
  while (1) {
    PasswordTestData test_data = password_test_data[i++];
    if (test_data.cipher_text_length == 0)
      break;

    CryptoMasterPasswordEncryption encryptor;
    std::vector<uint8_t> plain(
      test_data.plain, test_data.plain + strlen(test_data.plain));
    std::vector<uint8_t> encryption_key(
      test_data.encryption_key, test_data.encryption_key + strlen(test_data.encryption_key));
    std::vector<uint8_t> ciphertext;
    EncryptBlob(encryptor,
                plain,
                encryption_key,
                test_data.cipher,
                test_data.cipher_text_length,
                ciphertext);
    DecryptBlob(encryptor,
                plain,
                encryption_key,
                ciphertext);
  }
}

TEST(PasswordTest, BlobWithTrailingData) {
  PasswordTestData test_data = password_test_data[0];
  ASSERT_NE(test_data.cipher_text_length,0);

  CryptoMasterPasswordEncryption encryptor;
  std::vector<uint8_t> plain(
    test_data.plain, test_data.plain + strlen(test_data.plain));
  std::vector<uint8_t> encryption_key(
    test_data.encryption_key, test_data.encryption_key + strlen(test_data.encryption_key));
  std::vector<uint8_t> ciphertext;
  EncryptBlob(encryptor,
              plain,
              encryption_key,
              test_data.cipher,
              test_data.cipher_text_length,
              ciphertext);

  // blob contains fields describing data sizes, trailing characters should
  // be ignored
  ciphertext.push_back(std::numeric_limits<uint8_t>::max());

  DecryptBlob(encryptor,
              plain,
              encryption_key,
              ciphertext);
}

