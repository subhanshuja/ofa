// Copyright (c) 2014 Opera Software ASA. All rights reserved.

#include "net/turbo/turbo_certificate.h"

#include "crypto/sha2.h"
#include "net/base/hash_value.h"
#include "net/base/net_errors.h"
#include "net/cert/asn1_util.h"
#include "net/cert/x509_certificate.h"

namespace net {

static const uint8_t kTurboRootCA[] = {
#include "net/turbo/turbo_root_cert.h"
};

scoped_refptr<X509Certificate> TurboCertificate::Create() {
  return X509Certificate::CreateFromBytes(
      reinterpret_cast<const char*>(kTurboRootCA), sizeof(kTurboRootCA));
}

// We should probably just extract this offline and hardcode the hash value.
HashValue TurboCertificate::PublicKeyHash() {
  scoped_refptr<X509Certificate> cert = Create();

  base::StringPiece der_bytes(
      reinterpret_cast<const char*>(kTurboRootCA), sizeof(kTurboRootCA));
  base::StringPiece spki_bytes;
  if (!asn1::ExtractSPKIFromDERCert(der_bytes, &spki_bytes))
    DCHECK(false);

  HashValue sha256(HASH_VALUE_SHA256);
  crypto::SHA256HashString(spki_bytes, sha256.data(), crypto::kSHA256Length);
  return sha256;
}

bool TurboCertificate::ContainsTurboCA(const HashValueVector& hashes) {
  HashValueVector::const_iterator it = std::find(
      hashes.begin(), hashes.end(), PublicKeyHash());
  return it != hashes.end();
}

#ifndef OS_ANDROID

int TurboCertificate::Verify(X509Certificate* cert,
                             const std::string& hostname,
                             CertVerifyResult* verify_result) {
  // Note: This currently requires OpenSSL.
  //
  // An alternative is if the CertVerifyProc implementation returns true from
  // SupportsAdditionalTrustAnchors. Then we can verify with the Turbo CA as
  // an additional trust anchor, then check that the Turbo CA is among the
  // public key hashes. But only NSS supports that....
  //
  // It may be possible to verify by concatenating the certificate with our
  // CA as if it was sent by the server. We should get back something like
  // the stuff for a self-signed certificate, and the Turbo CA listed among
  // the returned certificate chain. But at least Android will instead fail
  // completely, and not return *any* information.

  LOG(INFO) << "Turbo CA validation not implemented for this platform";

  return ERR_FAILED;
}

#endif

}  // namespace net
