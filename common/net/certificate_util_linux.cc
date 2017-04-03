// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA.

#include "common/net/certificate_util.h"

#include "url/gurl.h"

namespace opera {
namespace cert_util {

CertificateRevocationCheckChains GetCertificateRevocationCheckChains(
    const net::X509Certificate& cert,
    bool verify_is_ev) {
  NOTIMPLEMENTED();
  return CertificateRevocationCheckChains();
}

std::vector<GURL> GetOCSPURLs(
    const net::X509Certificate::OSCertHandle& subject,
    const net::X509Certificate::OSCertHandle& issuer) {
  NOTIMPLEMENTED();
  return std::vector<GURL>();
}

std::vector<GURL> GetCRLURLs(const net::X509Certificate::OSCertHandle& cert) {
  NOTIMPLEMENTED();
  return std::vector<GURL>();
}

bool SetRevocationCheckResponse(const net::X509Certificate::OSCertHandle& cert,
                                const std::string& data,
                                bool is_ocsp_response) {
  NOTIMPLEMENTED();
  return false;
}

bool GetTimeValidRevocationCheckResponseFromCache(const GURL& url,
                                                  bool is_ocsp,
                                                  std::string* data) {
  NOTIMPLEMENTED();
  return false;
}

bool IsTimeValidRevocationCheckResponse(const std::string& data,
                                        bool is_ocsp_response) {
  NOTIMPLEMENTED();
  return false;
}

}  // namespace cert_util
}  // namespace opera
