// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA.
#ifndef COMMON_NET_CERTIFICATE_UTIL_H_
#define COMMON_NET_CERTIFICATE_UTIL_H_

#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "content/public/browser/ssl_status.h"
#include "net/cert/x509_certificate.h"

class GURL;

namespace opera {
namespace cert_util {

// The below types are used by GetCertificateRevocationCheckChains().
// The handles contained by the vector have to be freed by
// GetCertificateRevocationCheckChains()'s callers with
// net::X509Certificate::FreeOSCertHandle().
// See GetCertificateRevocationCheckChains().
using CertificateChain = std::vector<net::X509Certificate::OSCertHandle>;
using CertificateRevocationCheckChains = std::vector<CertificateChain>;

// Gets the name of the CA responsible for the certificate
std::string GetCertificateRootIssuer(const net::X509Certificate& certificate);

// Gets a (technical) string with connection details
// Example: "TLS 1.2 AES_128_GCM ECDHE_ECDSA (P-256)"
// Returns IDS_CONN_NO_ENCRYPTION if called on connections without encryption.
// Unavailable if the architecture does not compile common strings (e.g. iOS)
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
std::string GetConnectionStatusInfo(const content::SSLStatus& ssl_status);
#endif

// Gets the certificate chains to be used for revocation checking purposes.
// Caller is responsible for freeing of the returned
// net::X509Certificate::OSCertHandle with
// net::X509Certificate::FreeOSCertHandle().
// Note that chai's root is the last element of every chain.
CertificateRevocationCheckChains GetCertificateRevocationCheckChains(
    const net::X509Certificate& cert,
    bool verify_is_ev);

// Gets list of the OCSP responser URLs for the certificate.
std::vector<GURL> GetOCSPURLs(const net::X509Certificate::OSCertHandle& subject,
                              const net::X509Certificate::OSCertHandle& issuer);

// Gets list of the CRL download URLs for the certificate.
std::vector<GURL> GetCRLURLs(const net::X509Certificate::OSCertHandle& cert);

// Sets the revocation check response for the certificate. |is_ocsp_response|
// being true means the data is an OCSP response data. Otherwise it's a CRL.
bool SetRevocationCheckResponse(const net::X509Certificate::OSCertHandle& cert,
                                const std::string& data,
                                bool is_ocsp_response);

// Gets cached OCSP response or CRL list (depending on |is_ocsp|) for the URL.
// The response must be time valid (not expired i.e. NextUpdate time didn't
// pass). If the response isn't there or it expired this function returns false.
// Otherwise it returns true and sets the data in |data|.
bool GetTimeValidRevocationCheckResponseFromCache(const GURL& url,
                                                  bool is_ocsp,
                                                  std::string* data);

// Checks if the given revocation check response is time valid (not expired);
// If |is_ocsp_response| is true the given data represent OCSP response.
// Otherwise it's a CRL.
bool IsTimeValidRevocationCheckResponse(const std::string& data,
                                        bool is_ocsp_response);

}  // namespace cert_util
}  // namespace opera
#endif  // COMMON_NET_CERTIFICATE_UTIL_H_
