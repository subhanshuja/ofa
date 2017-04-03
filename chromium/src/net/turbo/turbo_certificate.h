// Copyright (c) 2014 Opera Software ASA. All rights reserved.

#ifndef NET_TURBO_TURBO_CERTIFICATE_H_
#define NET_TURBO_TURBO_CERTIFICATE_H_

#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "net/base/hash_value.h"
#include "net/cert/x509_certificate.h"

namespace net {

class CRLSet;
class CertVerifyResult;

class TurboCertificate {
 public:
  // The returned pointer must be stored in a scoped_refptr<X509Certificate>.
  static scoped_refptr<X509Certificate> Create();

  static HashValue PublicKeyHash();

  static bool ContainsTurboCA(const HashValueVector& public_keys);

  static int Verify(X509Certificate* cert,
                    const std::string& hostname,
                    CertVerifyResult* verify_result);

 private:
  DISALLOW_COPY_AND_ASSIGN(TurboCertificate);
};

}  // namespace net

#endif  // NET_TURBO_TURBO_CERTIFICATE_H_
