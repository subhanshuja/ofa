// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2008-2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

/**
 * This file implements the libcrypto 3DES algorithm towards the openssl
 * library.
 */

#include "migration_crypto_symmetric_algorithm3DES.h"

#include <cstring>

#include "openssl_des/des_locl.h"
#include "openssl_des/des.h"

using namespace ::opera::presto_port::crypto;

#define DES_ENCRYPT 1
#define DES_DECRYPT 0

CryptoSymmetricAlgorithm3DES::CryptoSymmetricAlgorithm3DES(int key_size)
    : m_key_size(key_size) {}

CryptoSymmetricAlgorithm3DES::~CryptoSymmetricAlgorithm3DES() {
  memset(&m_key1, 0, sizeof(DES_key_schedule));
  memset(&m_key2, 0, sizeof(DES_key_schedule));
  memset(&m_key3, 0, sizeof(DES_key_schedule));
}

CryptoSymmetricAlgorithm* CryptoSymmetricAlgorithm::Create3DES(int key_size) {
  if (key_size != 16 && key_size != 24)
    return NULL;

  return new CryptoSymmetricAlgorithm3DES(key_size);
}

void CryptoSymmetricAlgorithm3DES::Encrypt(
    const uint8_t* source, uint8_t* target) {
  CryptFunction3DES(source, target, DES_ENCRYPT);
}

void CryptoSymmetricAlgorithm3DES::Decrypt(
    const uint8_t* source, uint8_t* target) {
  CryptFunction3DES(source, target, DES_DECRYPT);
}

bool CryptoSymmetricAlgorithm3DES::SetEncryptKey(const uint8_t* key) {
  if (!key)
    return false;

  DES_cblock des_key1, des_key2, des_key3;

  memcpy(des_key1, key, 8);
  memcpy(des_key2, key + 8, 8);

  DES_set_key(&des_key1, &m_key1);
  DES_set_key(&des_key2, &m_key2);

  if (m_key_size == 24) {
    memcpy(des_key3, key + 16, 8);
    DES_set_key(&des_key3, &m_key3);
  } else
    DES_set_key(&des_key1, &m_key3);

  return true;
}

bool CryptoSymmetricAlgorithm3DES::SetDecryptKey(const uint8_t* key) {
  return SetEncryptKey(key);
}

void CryptoSymmetricAlgorithm3DES::CryptFunction3DES(
    const unsigned char* source,
    unsigned char* target,
    int direction) {
  DES_LONG l0, l1;
  DES_LONG ll[2];

  c2l(source, l0);
  c2l(source, l1);
  ll[0] = l0;
  ll[1] = l1;
  if (direction)
    DES_encrypt3(ll, &m_key1, &m_key2, &m_key3);
  else
    DES_decrypt3(ll, &m_key1, &m_key2, &m_key3);

  l0 = ll[0];
  l1 = ll[1];
  l2c(l0, target);
  l2c(l1, target);
}

int CryptoSymmetricAlgorithm3DES::GetKeySize() const { return m_key_size; }

int CryptoSymmetricAlgorithm3DES::GetBlockSize() const { return 8; }
