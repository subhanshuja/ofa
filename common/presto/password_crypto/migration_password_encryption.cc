// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2011-2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "migration_password_encryption.h"

#include <stdint.h>

#include "crypto/migration_encryption_cbc.h"
#include "base/logging.h"
#include "base/md5.h"
#include "base/sha1.h"
#include "base/sys_byteorder.h"

using namespace ::opera::presto_port;
using namespace ::opera::presto_port::crypto;

const char password_verification[] = "Opera Email Password Verification";

CryptoMasterPasswordEncryption::CryptoMasterPasswordEncryption()
    : m_des_cbc(CryptoStreamEncryptionCBC::Create(
          CryptoSymmetricAlgorithm::Create3DES(24),
          CryptoStreamEncryptionCBC::PADDING_STRATEGY_PCKS15)) {}

CryptoMasterPasswordEncryption::~CryptoMasterPasswordEncryption() {}

bool CryptoMasterPasswordEncryption::EncryptPassword(
    std::vector<uint8_t>& encrypted_password_blob,
    const std::vector<uint8_t>& password_to_encrypt,
    const std::vector<uint8_t>& master_password) {
  // Format: salt length data (4 bytes, network encoded) | salt | encrypted data
  // length data (4 bytes, network encoded) | encrypted data

  encrypted_password_blob.clear();

  int encrypted_data_length = m_des_cbc->CalculateEncryptedTargetLength(
      static_cast<int>(password_to_encrypt.size()));

  std::vector<uint8_t> encrypted_password;
  std::vector<uint8_t> salt;
  std::vector<uint8_t> seed_vector(
      password_verification,
      password_verification + strlen(password_verification));
  if (!Encrypt(encrypted_password,
               password_to_encrypt,
               salt,
               8,
               master_password,
               seed_vector))
    return false;

  /* puts the salt length and encrypted password length into the blob */

  uint32_t big_endian_8 = base::HostToNet32(8);
  const uint8_t* big_endian_8_data = (const uint8_t*)&big_endian_8;

  encrypted_password_blob.insert(encrypted_password_blob.end(),
                                 big_endian_8_data,
                                 big_endian_8_data + sizeof(big_endian_8));
  encrypted_password_blob.insert(
      encrypted_password_blob.end(), salt.begin(), salt.end());

  uint32_t big_endian_encrypted_password_length =
      base::HostToNet32(encrypted_data_length);
  const uint8_t* big_endian_encrypted_password_length_data =
      (const uint8_t*)&big_endian_encrypted_password_length;

  encrypted_password_blob.insert(
      encrypted_password_blob.end(),
      big_endian_encrypted_password_length_data,
      big_endian_encrypted_password_length_data +
          sizeof(big_endian_encrypted_password_length));

  encrypted_password_blob.insert(encrypted_password_blob.end(),
                                 encrypted_password.begin(),
                                 encrypted_password.end());

  return true;
}

bool CryptoMasterPasswordEncryption::DecryptPassword(
    std::vector<uint8_t>& decrypted_password,
    const std::vector<uint8_t>& password_blob,
    const std::vector<uint8_t>& master_password) {
  // Format: salt length data (4 bytes, network encoded) | salt | encrypted data
  // length data (4 bytes, network encoded) | encrypted data

  // size of field describing salt length
  const int kSaltLengthSize = 4;
  const int kMinimumSaltSize = 8;
  // size of field describing encrypted data length
  const int kEncryptedDataLengthSize = 4;
 
  if (password_blob.size() < kSaltLengthSize + kMinimumSaltSize +
                                 kEncryptedDataLengthSize +
                                 (size_t)m_des_cbc->GetBlockSize())
    return false;

  const uint32_t* in_salt_length32 =
      reinterpret_cast<const uint32_t*>(&password_blob[0]);
  size_t in_salt_length = base::NetToHost32(*in_salt_length32);

  /* We cannot trust salt_length, must check it makes sense to avoid buffer
   * overflow attacks. */
  if (in_salt_length >
          password_blob.size() - kSaltLengthSize - kEncryptedDataLengthSize -
              m_des_cbc->GetBlockSize() /* minimum encrypted data length */
      ||
      in_salt_length < static_cast<size_t>(kMinimumSaltSize))
    return false;

  const int salt_pos = kSaltLengthSize;
  const int length_data_pos = salt_pos + static_cast<int>(in_salt_length);
  const int encrypted_data_pos = length_data_pos + kEncryptedDataLengthSize;

  std::vector<uint8_t> in_salt;
  in_salt.insert(in_salt.end(),
                 password_blob.begin() + salt_pos,
                 password_blob.begin() + salt_pos + in_salt_length);

  const uint32_t* encrypted_data_length32 =
      reinterpret_cast<const uint32_t*>(&password_blob[length_data_pos]);
  const size_t encrypted_data_length = base::NetToHost32(*encrypted_data_length32);

  /* We cannot trust encrypted password length, must check it makes sense to
   * avoid buffer overflow attacks. */
  if (encrypted_data_length > password_blob.size() - kSaltLengthSize -
                                  kEncryptedDataLengthSize - in_salt_length ||
      encrypted_data_length < (unsigned int)m_des_cbc->GetBlockSize())
    return false;

  std::vector<uint8_t> encrypted_data(
      password_blob.begin() + encrypted_data_pos,
      password_blob.begin() + encrypted_data_pos + encrypted_data_length);
  std::vector<uint8_t> seed_vector(
      password_verification,
      password_verification + strlen(password_verification));
  return Decrypt(decrypted_password,
                 encrypted_data,
                 in_salt,
                 master_password,
                 seed_vector);
}

bool CryptoMasterPasswordEncryption::Encrypt(
    std::vector<uint8_t>& encrypted_data,
    const std::vector<uint8_t>& password_to_encrypt,
    std::vector<uint8_t>& salt,
    int salt_length,
    const std::vector<uint8_t>& master_password,
    const std::vector<uint8_t>& plainseed) {
  encrypted_data.clear();

  int temp_encrypted_data_length = m_des_cbc->CalculateEncryptedTargetLength(
      static_cast<int>(password_to_encrypt.size()));
  std::unique_ptr<uint8_t[]> temp_encrypted_data(new uint8_t[temp_encrypted_data_length]);

  CalculateSalt(
      password_to_encrypt, master_password, salt, salt_length, plainseed);

  CalculatedKeyAndIV(master_password, salt);

  /* Puts the encrypted password into the blob. */
  m_des_cbc->Encrypt(
      (password_to_encrypt.size() ? &password_to_encrypt[0] : NULL),
      temp_encrypted_data.get(),
      static_cast<int>(password_to_encrypt.size()),
      temp_encrypted_data_length);

  encrypted_data.insert(encrypted_data.begin(),
                        temp_encrypted_data.get(),
                        temp_encrypted_data.get() + temp_encrypted_data_length);

  return true;
}

bool CryptoMasterPasswordEncryption::Decrypt(
    std::vector<uint8_t>& decrypted_data,
    const std::vector<uint8_t>& encrypted_data,
    const std::vector<uint8_t>& in_salt,
    const std::vector<uint8_t>& password,
    const std::vector<uint8_t>& plainseed) {
  DCHECK(!encrypted_data.empty());
  decrypted_data.clear();

  std::vector<uint8_t> temp_decrypted_data(encrypted_data.size());

  std::vector<uint8_t> calculated_salt;

  CalculatedKeyAndIV(password, in_salt);

  if (!m_des_cbc->Decrypt(&encrypted_data[0],
                     &temp_decrypted_data[0],
                     static_cast<int>(encrypted_data.size())))
    return false;

  int temp_decrypted_data_length = 0;
  if (!m_des_cbc->CalculateDecryptedTargetLength(
           temp_decrypted_data_length,
           &temp_decrypted_data[0],
           static_cast<int>(encrypted_data.size())))
    return false;

  decrypted_data.insert(
      decrypted_data.end(),
      temp_decrypted_data.begin(),
      temp_decrypted_data.begin() + temp_decrypted_data_length);

  /* Based on the decrypted data, we can re-calculate the salt, and check that
   * it matches the salt given in in_data */
  CalculateSalt(decrypted_data,
                password,
                calculated_salt,
                static_cast<int>(in_salt.size()),
                plainseed);

  if (calculated_salt != in_salt)
    return false;

  return true;
}

void CryptoMasterPasswordEncryption::BytesToKey(
    const std::vector<uint8_t>& in_data,
    const std::vector<uint8_t>& salt,
    int hash_iteration_count,
    std::vector<uint8_t>& out_key,
    int out_key_length,
    std::vector<uint8_t>& out_iv,
    int out_iv_length) {
  out_key.assign(out_key_length, 0);
  out_iv.assign(out_iv_length, 0);

  base::MD5Context mdt_ctx;

  const int hasher_size = 16;
  uint8_t previous_iteration_result[hasher_size];

  int hash_count = 1;

  int current_key_data_generated_length = 0;
  int current_iv_data_generated_length = 0;

  while (current_key_data_generated_length + current_iv_data_generated_length <
         out_key_length + out_iv_length) {

    for (int i = 0; i < hash_count; i++) {
      base::MD5Init(&mdt_ctx);

      // Do not hash the result of previous iteration before actual having one.
      if (current_key_data_generated_length > 0 ||
          current_iv_data_generated_length || i > 0)
        base::MD5Update(&mdt_ctx,
                        base::StringPiece(reinterpret_cast<const char*>(
                                              previous_iteration_result),
                                          hasher_size));

      base::StringPiece in_data_string_piece =
          in_data.size()
              ? base::StringPiece(reinterpret_cast<const char*>(&in_data[0]),
                                  in_data.size())
              : base::StringPiece(std::string());
      base::MD5Update(&mdt_ctx, in_data_string_piece);

      base::StringPiece salt_string_piece =
          salt.size() > 0
              ? base::StringPiece(reinterpret_cast<const char*>(&salt[0]),
                                  salt.size())
              : base::StringPiece(std::string());
      base::MD5Update(&mdt_ctx, salt_string_piece);

      base::MD5Digest digest;
      base::MD5Final(&digest, &mdt_ctx);

      memcpy(previous_iteration_result, &digest.a, 16);
    }

    int hash_index = 0;
    if (current_key_data_generated_length < out_key_length) {
      while (current_key_data_generated_length < out_key_length &&
             hash_index < hasher_size)
        out_key[current_key_data_generated_length++] =
            previous_iteration_result[hash_index++];
    }

    if (current_key_data_generated_length >= out_key_length) {
      while (current_iv_data_generated_length < out_iv_length &&
             hash_index < hasher_size)
        out_iv[current_iv_data_generated_length++] =
            previous_iteration_result[hash_index++];
    }
  }
}

bool CryptoMasterPasswordEncryption::CalculateSalt(
    const std::vector<uint8_t>& in_data,
    const std::vector<uint8_t>& password,
    std::vector<uint8_t>& salt,
    int salt_length,
    const std::vector<uint8_t>& plainseed) const {
  if (salt_length > 64)
    return false;

  uint8_t digest_result[64]; /* ARRAY OK 2012-01-20 haavardm  */

  std::vector<uint8_t> hash_string(password.begin(), password.end());
  hash_string.insert(hash_string.end(), plainseed.begin(), plainseed.end());
  hash_string.insert(hash_string.end(), in_data.begin(), in_data.end());

  base::SHA1HashBytes(&hash_string[0], hash_string.size(), digest_result);

  salt.assign(digest_result, digest_result + salt_length);

  return true;
}

bool CryptoMasterPasswordEncryption::CalculatedKeyAndIV(
    const std::vector<uint8_t>& password,
    const std::vector<uint8_t>& salt) const {
  std::vector<uint8_t> out_key;
  std::vector<uint8_t> out_iv;

  BytesToKey(password,
             salt,
             1,
             out_key,
             m_des_cbc->GetKeySize(),
             out_iv,
             m_des_cbc->GetBlockSize());

  m_des_cbc->SetIV(&out_iv[0]);
  return m_des_cbc->SetKey(&out_key[0]);
}
