// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CRYPTO_SYMMETRIC_ALGORITHM_H
#define CRYPTO_SYMMETRIC_ALGORITHM_H

#include <stdint.h>

namespace opera {
namespace presto_port {
namespace crypto {

/**
 * @file CryptoSymmetricAlgorithm.h
 *
 * Symmetric crypto algorithm porting interface. There is a tweak that
 *decides
 * if the algorithm is implemented by core(libopeay)  or the platform.
 *
 * More ciphers can be added by adding CreateCIPHERNAME() functions,
 * in the same way the hashes are implemented.
 */

// Maximum known cipher block size in bytes.
// It is 32 bytes == 256 bits, size of AES256.
#define CRYPTO_MAX_CIPHER_BLOCK_SIZE 32

/** @class CryptoSymmetricAlgorithm.
 *
 * Block encryption algorithm.
 *
 * Use to encrypt blocks of data with a symmetric algorithm.
 *
 * Don't use these encryption algorithms directly if you don't
 * really know what you are doing, as it is unsafe without a proper
 * encryption mode.
 *
 * Wrap it in CryptoStreamEncryptionCBC or CryptoStreamEncryptionCFB instead.
 *
 * @see CryptoStreamEncryptionCBC or CryptoStreamEncryptionCFB.
 */
class CryptoSymmetricAlgorithm {
 public:
  virtual ~CryptoSymmetricAlgorithm() {};

#ifdef CRYPTO_ENCRYPTION_AES_SUPPORT  // import API_CRYPTO_ENCRYPTION_AES

  /** Create an AES encryption algorithm.
   *
   * @param key_size The key size in bytes for AES.
   *                  Legal values are 16, 24 and 32.
   */
  static CryptoSymmetricAlgorithm* CreateAES(int key_size = 16);
#endif  // CRYPTO_ENCRYPTION_AES_SUPPORT

  /** Create an 3DES encryption algorithm.
   *
   * @param key_size The key size in bytes for 3DES.
   *                  Legal values are 16 and 24.
   * @return the algorithm, or NULL for OOM or wrong key size.
   */
  static CryptoSymmetricAlgorithm* Create3DES(int key_size = 24);

  /** Encrypt one block of data.
   *
   * Block size is given by GetBlockSize();
   *
   *  @param source Preallocated source array of length GetBlockSize()
   *  @param target Preallocated target array of length GetBlockSize()
   */
  virtual void Encrypt(const uint8_t* source, uint8_t* target) = 0;

  /** Decrypt one block of data.
   *
   * Block size is given by GetBlockSize().
   *
   *  @param source Preallocated source array of length GetBlockSize()
   *  @param target Preallocated target array of length GetBlockSize()
   */
  virtual void Decrypt(const uint8_t* source, uint8_t* target) = 0;

  /** Sets the encryption key.
   *
   * @param key The key of length GetKeySize();
   * @return true or false_NO_MEMORY.
   */
  virtual bool SetEncryptKey(const uint8_t* key) = 0;

  /** Sets the decryption key.
   *
   * @param key The key of length GetKeySize().
   * @return true or false_NO_MEMORY.
   */
  virtual bool SetDecryptKey(const uint8_t* key) = 0;

  /** Get key size.
   *
   * @return The key size in bytes.
   * */
  virtual int GetKeySize() const = 0;

  /** Get block size.
   *
   * @return The block size in bytes.
   * */
  virtual int GetBlockSize() const = 0;
};

}  // namespace crypto
}  // namespace presto_port
}  // namespace opera

#endif /* CRYPTO_SYMMETRIC_ALGORITHM_H */
