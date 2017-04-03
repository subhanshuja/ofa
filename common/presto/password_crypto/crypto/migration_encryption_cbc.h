// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2008-2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CRYPTO_STREAM_ENCRYPTION_CBC_H_
#define CRYPTO_STREAM_ENCRYPTION_CBC_H_

#include <stdint.h>

#include <memory>

#include "migration_crypto_symmetric_algorithm.h"

namespace opera {
namespace presto_port {
namespace crypto {

class CryptoStreamEncryptionCBC  // import API_CRYPTO_STREAM_ENCRYPTION_CBC
    {
 public:
  enum PaddingStrategy {
    PADDING_STRATEGY_NONE,   // Lengths if source and target must be equal and a
                             // multiple of the block size.
    PADDING_STRATEGY_PCKS15  // Enables the API to encrypt arbitrary long plain
                             // texts.
  };

  static CryptoStreamEncryptionCBC* Create(
      CryptoSymmetricAlgorithm* cipher_algorithm,
      PaddingStrategy padding_strategy =
          PADDING_STRATEGY_NONE);  // Takes over cipher_algorithm

  virtual bool Encrypt(const uint8_t* source,
                       uint8_t* target,
                       int source_length,
                       int target_length = 0);

  virtual bool Decrypt(const uint8_t* source, uint8_t* target, int length);

  virtual bool SetKey(const uint8_t* key);

  virtual void SetIV(const uint8_t* iv);

  virtual int GetKeySize() const;

  virtual int GetBlockSize() const;

  const uint8_t* GetState() const { return m_state.get(); }

  int CalculateEncryptedTargetLength(int source_length) const;
  bool CalculateDecryptedTargetLength(int& decrypted_target_length,
                                      const uint8_t* decrypted_source,
                                      int source_length) const;

  virtual ~CryptoStreamEncryptionCBC();

 private:
  CryptoStreamEncryptionCBC(CryptoSymmetricAlgorithm* cipher_algorithm,
                            PaddingStrategy padding_strategy);

  std::unique_ptr<CryptoSymmetricAlgorithm> m_algorithm;
  PaddingStrategy m_padding_strategy;
  std::unique_ptr<uint8_t[]> m_state;
  std::unique_ptr<uint8_t[]> m_key;
  unsigned int m_state_counter;
};

}  // namespace crypto
}  // namespace presto_port
}  // namespace opera

#endif /* CRYPTO_STREAM_ENCRYPTION_CBC_H_ */
