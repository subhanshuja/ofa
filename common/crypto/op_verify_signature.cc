// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/crypto/op_verify_signature.h"

#include <memory>
#include <string>

#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/sys_byteorder.h"
#include "crypto/signature_verifier.h"
#include "third_party/modp_b64/modp_b64.h"

namespace {

bool OpVerifySignatureInternal(
    const std::string& text,
    const std::string& prefix,
    const uint8_t* public_key_info,
    int public_key_info_len,
    crypto::SignatureVerifier::SignatureAlgorithm algorithm) {
#if defined(DISABLE_CRYPTO_SIGNATURE_CHECK)
  LOG(WARNING) << "Signature verification for resources is turned off.";
  return true;
#else
  const size_t p_len = prefix.length();

  if (text.length() <= p_len)
    return false;

  if (text.compare(0, p_len, prefix) != 0)
    return false;

  const size_t sig_end = text.find('\n', p_len);

  if (sig_end == std::string::npos)
    return false;

  std::string sig(text, p_len, sig_end - p_len);
  modp_b64_decode(sig);  // |sig| is an in-out parameter

  if (sig.empty())
    return false;

  crypto::SignatureVerifier verifier;
  if (!verifier.VerifyInit(algorithm,
                           reinterpret_cast<const uint8_t*>(sig.data()),
                           base::checked_cast<int>(sig.length()),
                           public_key_info,
                           public_key_info_len)) {
    return false;
  }

  verifier.VerifyUpdate(
      reinterpret_cast<const uint8_t*>(text.data() + sig_end + 1),
      base::checked_cast<int>(text.length() - sig_end - 1));

  return verifier.VerifyFinal();
#endif  // defined(DISABLE_CRYPTO_SIGNATURE_CHECK)
}
}  // namespace

bool OpVerifySignature(const std::string& text,
                       const std::string& prefix,
                       const uint8_t* public_key_info,
                       int public_key_info_len) {
  return OpVerifySignatureInternal(text, prefix, public_key_info,
                                   public_key_info_len,
                                   crypto::SignatureVerifier::RSA_PKCS1_SHA1);
}

bool OpVerifySignatureSHA256(const std::string& text,
                             const std::string& prefix,
                             const uint8_t* public_key_info,
                             int public_key_info_len) {
  return OpVerifySignatureInternal(text, prefix, public_key_info,
                                   public_key_info_len,
                                   crypto::SignatureVerifier::RSA_PKCS1_SHA256);
}
