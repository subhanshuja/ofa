// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_CRYPTO_OP_VERIFY_SIGNATURE_H_
#define COMMON_CRYPTO_OP_VERIFY_SIGNATURE_H_

#include <stdint.h>
#include <string>

/** Verifies a signed text file.
 *
 * Note that this algorithm is similar but not backward compatible with
 * the verifying algorithm in opera 12.x.
 *
 * For details how to sign text please read /common/crypto/signingtools/README.txt.
 *
 * First line of input text begins with the prefix parameter, with the rest
 * of the line being a Base64 encoded signature. The signed content starts
 * after the LF.
 *
 * @param text    Full text of file.
 * @param prefix  Prefix, after which the Base64 encoded signature follows.
 * @param public_key_info  RSA public key for signature verification.
 * @param public_key_info_len  the byte length of public_key_info.
 *
 * @return Returns true if the verification succeded, false on any error.
 */
bool OpVerifySignature(const std::string& text,
                       const std::string& prefix,
                       const uint8_t* public_key_info,
                       int public_key_info_len);

/** Verifies a signed text file.
 *
 * This function use SHA256 {OID sha256WithRSAEncryption PARAMETERS NULL}
 *
 * First line of input text begins with the prefix parameter, with the rest
 * of the line being a Base64 encoded signature. The signed content starts
 * after the LF.
 *
 * @param text    Full text of file.
 * @param prefix  Prefix, after which the Base64 encoded signature follows.
 * @param public_key_info  RSA public key for signature verification.
 * @param public_key_info_len  the byte length of public_key_info.
 *
 * @return Returns true if the verification succeded, false on any error.
 */
bool OpVerifySignatureSHA256(const std::string& text,
                             const std::string& prefix,
                             const uint8_t* public_key_info,
                             int public_key_info_len);

#endif  // COMMON_CRYPTO_OP_VERIFY_SIGNATURE_H_
