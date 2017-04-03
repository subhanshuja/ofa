// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2008-2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CRYPTO_SYMMETRIC_ALGORITHM_3DES_H
#define CRYPTO_SYMMETRIC_ALGORITHM_3DES_H

#include "migration_crypto_symmetric_algorithm.h"
#include "openssl_des/des.h"
#include "base/compiler_specific.h"

namespace opera {
namespace presto_port {
namespace crypto {

class CryptoSymmetricAlgorithm3DES : public CryptoSymmetricAlgorithm {
 public:
  inline ~CryptoSymmetricAlgorithm3DES() override;

  void Encrypt(const uint8_t* source, uint8_t* target) override;
  void Decrypt(const uint8_t* source, uint8_t* target) override;

  bool SetEncryptKey(const uint8_t* key) override;
  bool SetDecryptKey(const uint8_t* key) override;

  int GetKeySize() const override;
  int GetBlockSize() const override;

  CryptoSymmetricAlgorithm3DES(int key_length);

  void CryptFunction3DES(const uint8_t* source, uint8_t* target, int direction);

 private:
  DES_key_schedule m_key1, m_key2, m_key3;
  int m_key_size;
};

}  // namespace crypto
}  // namespace presto_port
}  // namespace opera

#endif /* CRYPTO_SYMMETRIC_ALGORITHM_3DES_H */
