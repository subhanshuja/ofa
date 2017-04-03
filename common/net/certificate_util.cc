// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA.

#include "common/net/certificate_util.h"

#include "base/strings/string_number_conversions.h"
#include "net/ssl/ssl_cipher_suite_names.h"
#include "net/ssl/ssl_connection_status_flags.h"
#include "ui/base/l10n/l10n_util.h"

#include "opera_common/grit/product_free_common_strings.h"

namespace opera {
namespace cert_util {

// Unavailable if the architecture does not compile common strings (e.g. iOS)
#if !TARGET_IPHONE_SIMULATOR && !TARGET_OS_IPHONE
namespace {

std::string GetNoEncryptionString() {
  return l10n_util::GetStringUTF8(IDS_CONN_NO_ENCRYPTION);
}

}  // namespace

std::string GetConnectionStatusInfo(const content::SSLStatus& ssl_status) {
  if (ssl_status.security_style == content::SECURITY_STYLE_UNKNOWN)
    return GetNoEncryptionString();

  if (!ssl_status.certificate) {
    // Not HTTPS.
    DCHECK_EQ(ssl_status.security_style,
              content::SECURITY_STYLE_UNAUTHENTICATED);
    return GetNoEncryptionString();
  }

  if (ssl_status.security_bits <= 0) {
    DCHECK_NE(ssl_status.security_style,
              content::SECURITY_STYLE_UNAUTHENTICATED);
    return GetNoEncryptionString();
  }

  const uint16_t cipher_suite =
      net::SSLConnectionStatusToCipherSuite(ssl_status.connection_status);
  if (cipher_suite == 0)
    return GetNoEncryptionString();

  const int ssl_version =
      net::SSLConnectionStatusToVersion(ssl_status.connection_status);

  const char* ssl_version_str;
  net::SSLVersionToString(&ssl_version_str, ssl_version);
  std::string display_string(ssl_version_str);

  const char *key_exchange_str, *cipher_str, *mac_str;
  bool is_aead;
  net::SSLCipherSuiteToStrings(&key_exchange_str, &cipher_str, &mac_str,
                               &is_aead, cipher_suite);

  display_string.append(1, ' ').append(cipher_str);
  if (!is_aead && mac_str)
    display_string.append(1, ' ').append(mac_str);
  display_string.append(1, ' ').append(key_exchange_str);

  // Add brackets with key exchange group to the end
  if (ssl_status.key_exchange_group > 0) {
    display_string.append(" (");
    display_string.append(base::UintToString(ssl_status.key_exchange_group));
    display_string.append(")");
  }

  return display_string;
}
#endif

std::string GetCertificateRootIssuer(const net::X509Certificate& certificate) {
  std::string display_string;
  // we want the root certificate issuer
  const net::X509Certificate::OSCertHandles& intermediate_certificates =
      certificate.GetIntermediateCertificates();
  if (intermediate_certificates.size() > 0) {
    // last is the root CA certificate
    net::X509Certificate::OSCertHandle cert_handle =
        intermediate_certificates[intermediate_certificates.size() - 1];

    // we don't have intermediates for this one, so don't provide any
    net::X509Certificate::OSCertHandles empty;
    scoped_refptr<net::X509Certificate> cert(
        net::X509Certificate::CreateFromHandle(cert_handle, empty));

    // pick the first organization name (a root without it
    // is invalid and the code wouldn't get this far without it)
    if (cert->subject().organization_names.size()) {
      display_string = cert->subject().organization_names[0];
    } else if (!cert->issuer().GetDisplayName().empty()) {
      // use the display name if available
      display_string = cert->issuer().GetDisplayName();
    }
  }
  if (display_string.empty() &&
      !certificate.issuer().GetDisplayName().empty()) {
    display_string = certificate.issuer().GetDisplayName();
  }
  return display_string;
}

}  // namespace cert_util
}  // namespace opera
