// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/wand/wand_reader.h"

#include "base/strings/utf_string_conversions.h"

using opera::presto_port::CryptoMasterPasswordEncryption;
using opera::presto_port::CryptoMasterPasswordHandler;

namespace opera {
namespace common {
namespace migration {

WandReader::WandReader(const std::vector<uint8_t>& encryption_key,
                       std::istream* input,
                       bool input_is_big_endian) :
    BinaryReader(input, input_is_big_endian),
    encryption_key_(encryption_key),
    decryption_failed_(false) {
}

WandReader::~WandReader() {
}

base::string16 WandReader::ReadEncryptedString16(bool is_password) {
  int32_t length = Read<int32_t>();
  if (length == 0)
    return base::string16();
  std::vector<uint8_t> encrypted_password;
  for (int32_t i = 0; i < length && !IsEof(); ++i) {
    encrypted_password.push_back(Read<uint8_t>());
  }

  std::vector<uint8_t> decrypted_password;
  if (!password_decryptor_.DecryptPassword(
        decrypted_password,
        encrypted_password,
        is_password ?  // Only passwords may be encrypted using master key
          encryption_key_ :
          encryption_handler_.GetObfuscationPasswordEncryptionKey())) {
    decryption_failed_ = true;
    return base::string16();
  }

  base::string16 output;
  size_t decrypted_password_size = decrypted_password.size();
  /* Passwords are encoded without a trailing null char. All other encoded
   * strings (usernames, urls, etc.) have a null terminator.
   */
  bool has_terminating_nullchar = !is_password;
  /* If the string was encoded with trailing \0, we don't want it included
   * in the output as an extra character. Don't worry, this won't make anyone
   * read garbage memory. We're building a base::string16 from a vector, there's
   * no need to terminate.
   */
  if (decrypted_password_size > 0 && has_terminating_nullchar)
    decrypted_password_size -= sizeof(base::string16::value_type);

  for (size_t i = 0 ;
       i < decrypted_password_size;
       i += sizeof(base::string16::value_type)) {
    output.push_back(*reinterpret_cast<base::string16::value_type*>(
                       &decrypted_password[i]));
  }
  return output;
}

bool WandReader::IsFailed() const {
  return decryption_failed_ || BinaryReader::IsFailed();
}

}  // namespace migration
}  // namespace common
}  // namespace opera
