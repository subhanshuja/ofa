// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gmock/include/gmock/gmock.h"

#include <stdint.h>
#include <cstdio>
#include "migration_encryption_cbc.h"
#include "migration_crypto_symmetric_algorithm.h"
#include "base/numerics/safe_conversions.h"

using namespace ::testing;

using namespace ::opera::presto_port::crypto;

class CryptoTest : public ::testing::Test {
 protected:
  CryptoTest();
  ~CryptoTest() override;

  void createBogusKey(uint8_t key[], int size);
  void createBogusIv(uint8_t iv[], int size);

  CryptoStreamEncryptionCBC* alg_;
  int block_size_;
  int key_size_;
  uint8_t* key_;
  uint8_t* iv_;
};

CryptoTest::CryptoTest() {
  alg_ = CryptoStreamEncryptionCBC::Create(
      CryptoSymmetricAlgorithm::Create3DES(16));

  block_size_ = alg_->GetBlockSize();
  int key_size_ = alg_->GetKeySize();

  key_ = new uint8_t[key_size_];
  iv_ = new uint8_t[block_size_];

  createBogusKey(key_, key_size_);
  createBogusIv(iv_, block_size_);
}

CryptoTest::~CryptoTest() {
  delete[] key_;
  delete[] iv_;
  delete alg_;
}

void CryptoTest::createBogusKey(uint8_t key[], int size) {
  for (int i = 0; i < size; ++i) {
    key[i] = static_cast<uint8_t>(i) ^ 255;
  }
}

void CryptoTest::createBogusIv(uint8_t iv[], int size) {
  for (int i = 0; i < size; ++i) {
    iv[i] = static_cast<uint8_t>(i);
  }
}

TEST_F(CryptoTest, DES_ECB) {

  // Test encryption and decryption
  const char* data = "Dette skal krypteres. 32 bytes.";
  int data_len = base::checked_cast<int>(strlen(data))
                 + 1; // We also encrypt the '0/'

  uint8_t* target = new uint8_t[data_len];
  uint8_t* decrypted_target = new uint8_t[data_len];

  EXPECT_TRUE(alg_->SetKey(key_));

  alg_->SetIV(iv_);
  alg_->Encrypt(reinterpret_cast<const uint8_t*>(data), target, data_len);

  alg_->SetIV(iv_);
  EXPECT_TRUE(alg_->Decrypt(target, decrypted_target, data_len));

  // Check that decrypt(encrypt(data)) == data
  EXPECT_TRUE(memcmp(decrypted_target, data, data_len) == 0);

  delete[] target;
  delete[] decrypted_target;
}

TEST_F(CryptoTest, EncryptWithMissingKey) {

  const char* data = "Cos do zaszyfrowania. 32 bytes.";
  int data_len = base::checked_cast<int>(strlen(data))
                 + 1;  // We also encrypt the '0/'

  uint8_t* target = new uint8_t[data_len];

  EXPECT_FALSE(alg_->SetKey(NULL));
  EXPECT_FALSE(
      alg_->Encrypt(reinterpret_cast<const uint8_t*>(data), target, data_len));

  delete[] target;
}

TEST_F(CryptoTest, DecryptWithMissingKey) {

  const char* data = "Cos do zaszyfrowania. 32 bytes.";
  int data_len = base::checked_cast<int>(strlen(data))
                 + 1;  // We also encrypt the '0/'

  uint8_t* target = new uint8_t[data_len];
  uint8_t* decrypted_target = new uint8_t[data_len];

  alg_->SetIV(iv_);
  EXPECT_FALSE(alg_->Decrypt(target, decrypted_target, data_len));

  delete[] target;
  delete[] decrypted_target;
}

TEST_F(CryptoTest, EmptyMessageTest) {

  // Try to decrypt empty message
  std::vector<uint8_t> reference;
  const int reference_size = block_size_ * 2;
  for (int i = 0; i < reference_size; ++i) {
    reference.push_back(static_cast<uint8_t>(i));
  }

  std::vector<uint8_t> decrypted_target(reference.begin(), reference.end());

  EXPECT_TRUE(alg_->SetKey(key_));

  // Expected result:
  // 1) Decrypt doesn't try to read from source
  // 2) Decrypt doesn't write to target
  alg_->SetIV(iv_);
  EXPECT_FALSE(alg_->Decrypt(NULL, &decrypted_target[0], 0));

  EXPECT_EQ(reference, decrypted_target);
}

TEST_F(CryptoTest, IncorrectMessageLengthTest) {

  if (block_size_ == 1)
    return;  // in such case test makes no sense

  // Try to decrypt message with incorrect length (not composed of whole
  // blocks)
  const int data_len = block_size_ * 3;
  const int incorrect_data_len = data_len - block_size_ / 2;

  std::vector<uint8_t> source(incorrect_data_len);

  std::vector<uint8_t> reference;
  for (int i = 0; i < data_len; ++i) {
    reference.push_back(static_cast<uint8_t>(i));
  }

  std::vector<uint8_t> decrypted_target(reference.begin(), reference.end());

  EXPECT_TRUE(alg_->SetKey(key_));

  // Expected result: Decrypt does not try to process messages with incorrect
  // size and returns false
  alg_->SetIV(iv_);
  EXPECT_FALSE(
      alg_->Decrypt(&source[0], &decrypted_target[0], incorrect_data_len));
  EXPECT_EQ(reference, decrypted_target);
}
