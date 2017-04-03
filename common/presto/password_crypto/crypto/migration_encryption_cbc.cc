// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2008-2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "migration_encryption_cbc.h"

#include <cstring>

using namespace ::opera::presto_port::crypto;

CryptoStreamEncryptionCBC::CryptoStreamEncryptionCBC(
    CryptoSymmetricAlgorithm* cipher_algorithm,
    PaddingStrategy padding_strategy)
    : m_algorithm(cipher_algorithm),
      m_padding_strategy(padding_strategy),
      m_state_counter(0) {}

CryptoStreamEncryptionCBC::~CryptoStreamEncryptionCBC() {}

CryptoStreamEncryptionCBC* CryptoStreamEncryptionCBC::Create(
    CryptoSymmetricAlgorithm* cipher_algorithm,
    PaddingStrategy padding_strategy) {
  if (!cipher_algorithm)
    return NULL;
  int block_size = cipher_algorithm->GetBlockSize();

  CryptoStreamEncryptionCBC* temp_alg =
      new CryptoStreamEncryptionCBC(cipher_algorithm, padding_strategy);

  temp_alg->m_state.reset(new uint8_t[block_size]);

  return temp_alg;
}

bool CryptoStreamEncryptionCBC::Encrypt(const uint8_t* source,
                                        uint8_t* target,
                                        int source_length,
                                        int target_length) {
  // IMPROVE ME : Implement cipher stealing for arbitrary lengths

  if (target_length == 0 && m_padding_strategy == PADDING_STRATEGY_NONE)
    target_length = source_length;

  uint8_t padding_byte = target_length - source_length;

  if (!m_algorithm->SetEncryptKey(m_key.get()))
    return false;

  int block_size = GetBlockSize();

  for (int i = 0; i < target_length; i++) {
    uint8_t source_byte = i < source_length ? source[i] : padding_byte;

    m_state[m_state_counter] = m_state[m_state_counter] ^ source_byte;

    m_state_counter = (m_state_counter + 1) % block_size;

    if (!m_state_counter) {
      m_algorithm->Encrypt(m_state.get(), target + i + 1 - block_size);
      memmove(m_state.get(), target + i + 1 - block_size, block_size);
    }
  }
  return true;
}

bool CryptoStreamEncryptionCBC::Decrypt(const uint8_t* source,
                                        uint8_t* target,
                                        int length) {
  // Sensible results require non-empty message consisting of integral number of
  // blocks
  int block_size = GetBlockSize();
  if (length == 0 || (length % block_size != 0))
    return false;

  if (!m_algorithm->SetDecryptKey(m_key.get()))
    return false;

  for (int i = 0; i < length; i++) {
    if (!m_state_counter) {
      m_algorithm->Decrypt(source + i, target + i);
    }

    target[i] ^= m_state[m_state_counter];

    m_state[m_state_counter] = source[i];

    m_state_counter = (m_state_counter + 1) % block_size;
  }
  return true;
}

bool CryptoStreamEncryptionCBC::SetKey(const uint8_t* key) {
  if (!key)
    return false;

  if (!m_key)
    m_key.reset(new uint8_t[GetKeySize()]);

  memmove(m_key.get(), key, GetKeySize());
  return true;
}

void CryptoStreamEncryptionCBC::SetIV(const uint8_t* iv) {
  memcpy(m_state.get(), iv, m_algorithm->GetBlockSize());
  m_state_counter = 0;
}

int CryptoStreamEncryptionCBC::CalculateEncryptedTargetLength(
    int source_length) const {
  int block_size = GetBlockSize();

  switch (m_padding_strategy) {
    case PADDING_STRATEGY_NONE:
      return source_length;

    case PADDING_STRATEGY_PCKS15:
      /*
       * Pad the encrypted target with byte 'n' where n is the number
       * of padding bytes. Bytes are added until target_length == 0 mod
       * block_size
       *
       * If source length  == 0 mod block size, one block of padding
       * is added.
       */
      return source_length + (block_size - (source_length % block_size));

    default:
      return source_length;
  }
}

bool CryptoStreamEncryptionCBC::CalculateDecryptedTargetLength(
    int& decrypted_target_length,
    const uint8_t* decrypted_source,
    int source_length) const {
  decrypted_target_length = 0;
  bool status = true;
  switch (m_padding_strategy) {
    case PADDING_STRATEGY_NONE:
      decrypted_target_length = source_length;
      break;

    case PADDING_STRATEGY_PCKS15:
      if (source_length >= decrypted_source[source_length - 1])
        decrypted_target_length =
            source_length - decrypted_source[source_length - 1];
      else
        status = false;
      break;

    default:
      decrypted_target_length = source_length;
      break;
  }

  return status;
}

int CryptoStreamEncryptionCBC::GetKeySize() const {
  return m_algorithm->GetKeySize();
}

int CryptoStreamEncryptionCBC::GetBlockSize() const {
  return m_algorithm->GetBlockSize();
}
