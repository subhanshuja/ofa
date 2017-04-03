// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include <string>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/numerics/safe_conversions.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/test/test_browser_thread.h"
#include "crypto/sha2.h"
#include "net/base/net_errors.h"
#include "net/base/test_completion_callback.h"
#include "net/cert/asn1_util.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/crl_set.h"
#include "net/cert/crl_set_storage.h"
#include "net/cert/test_root_certs.h"
#include "net/cert/x509_certificate.h"
#include "net/log/net_log.h"
#include "net/url_request/test_url_request_interceptor.h"
#include "net/url_request/url_request_filter.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "common/net/opera_cert_verifier_delegate.h"

using net::X509Certificate;
using net::CertVerifier;
using net::BoundNetLog;

namespace opera {

namespace {

const char kExampleHostname[] = "test.example.com";
const char kExampleHostnameDomain[] = "example.com";

const char kExampleCertificate[] =
    "MIIDIjCCAgygAwIBAgIBAzALBgkqhkiG9w0BAQUwLDEQMA4GA1UEChMHQWNtZSBD"
    "bzEYMBYGA1UEAxMPSW50ZXJtZWRpYXRlIENBMB4XDTEzMDEwMTEwMDAwMFoXDTIz"
    "MTIzMTEwMDAwMFowLTEQMA4GA1UEChMHQWNtZSBDbzEZMBcGA1UEAxMQTGVhZiBj"
    "ZXJ0aWZpY2F0ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBANXKGXmO"
    "qatG8E63WG2zo5poEFKv8ACUrjS9tFAfoyaknhyQN1s96Nc7vJP7APvHSVSb8dCa"
    "8lGEe1mLvWbzrpJauWOMZKfQnuMMUNLPk53pShFXk8Her3taRB0KjCKmHcat6Y8W"
    "jU6R8dPx84L+9lXccvERB3Xsu+k6NYdDgV7cQ0q3fKEa1dLBQDlpfYmtZBsxNKjq"
    "nl4m/HHSxmvlwnMwP1mnNY2ppek9Q0G9VPIq4RUMNTBri/J3ylwHj1j0VHder86x"
    "wSunu8Dpfe8a1wPuj2etxuYdqeeRP0Hn1oYgjFOz2HkJ4ksVWtiSO2JPaOTL0KRO"
    "tn0+X7Ak6mJhz3sCAwEAAaNSMFAwDgYDVR0PAQH/BAQDAgCgMBMGA1UdJQQMMAoG"
    "CCsGAQUFBwMBMAwGA1UdEwEB/wQCMAAwGwYDVR0RBBQwEoIQdGVzdC5leGFtcGxl"
    "LmNvbTALBgkqhkiG9w0BAQUDggEBAFjD3ONN7HbGYpm6um3a5C/sAPj7KuP2pLw3"
    "yVMPcy6meY9r74cWVnuebawa7ItJcX3yERGkDV5uvpNr/stEG06ZKtLr2JGA18iH"
    "/cj6z8JoBgctYK5WxDxJTeMFPxsVqKnqhdiv0/W+tXEoI40E8cbh+wwbrFot4H/7"
    "TnlHKbOcJwl9PIQLWQoDxYapqpBJiQu8jg4usWftmb437nV/qfpilUQCHJkm+qcX"
    "YdLs4cpCK2mXj3HcG0F7kajWsoIF79ALPEajnXwGgdretlStl73CAwL/G2QXJUpM"
    "m4XBu28mOrW6my0Xub02sUNIKffaiI3O8Kx/A6eT4enBWBWzMCI=";

const char kExampleCertificateIntermediate[] =
    "MIIC/zCCAemgAwIBAgIBAjALBgkqhkiG9w0BAQUwJDEQMA4GA1UEChMHQWNtZSBD"
    "bzEQMA4GA1UEAxMHUm9vdCBDQTAeFw0xMzAxMDExMDAwMDBaFw0yMzEyMzExMDAw"
    "MDBaMCwxEDAOBgNVBAoTB0FjbWUgQ28xGDAWBgNVBAMTD0ludGVybWVkaWF0ZSBD"
    "QTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAM01UOcKaIDlK/ABK5MR"
    "DFD3I+HY0u1Imuo7ZJ+C+uStI5aooZsx0dZKsnnxwYADGEFUpTA6gr1XEJz9XTT9"
    "GdMhG8sG52ZA4SeJmIIt1y4NXAWadA1F3jJeeE6BtMhgl/CLKozgV/a521pTZB0n"
    "4JNH2ZPurPZ759KXsaaFN3X/qvePrpJOMAtWVP0y+Z082C6V9WQX/ybSZeKxeGyD"
    "XWek2K6Ja26zSzWlsQM8IJd57Qv43iWhOlBwQK6eBHWiai8VhFsIw+BVTkfbvHkl"
    "sC5YDbyqpvLuzea4AoxbALM9RNCmv7PnLp1GcN5F0b15vcDyRwtxKGCRwphzFS20"
    "sfMCAwEAAaM4MDYwDgYDVR0PAQH/BAQDAgAEMBMGA1UdJQQMMAoGCCsGAQUFBwMB"
    "MA8GA1UdEwEB/wQFMAMBAf8wCwYJKoZIhvcNAQEFA4IBAQC1Ziyh+HaKO2wGLdUe"
    "SyVcuG3uDn4JpENYZZPp2mxCLl10P3lhTeVyRdct/XOO4pj+jkrkEW6UXNmEycuh"
    "HPqV2RXBh3KYLmPfZ00EH9rXKWbsIOq2XXHdvFoWVYePUZ9ABQA7Ie50vDsRmhC6"
    "tOhebpDDIsrakvj7jnP9aZETSBEBWK70sow4VvClOypkXCWau/2UJzSvtCFMCCM8"
    "+z8Ibwe4BZ2FHXMO8IP0PJvMqv09+oKk3QEQnRAsxEdkyrS1br5Z0dKharXTCCNJ"
    "/E/U86VjteE0GZ2MMw+ORwGa6yrry/QaDO6OaNPBjv1Lk/9AjDoRK2KjwacTvSY3"
    "xYXF";

const char kExampleCertificateRoot[] =
    "MIIC9zCCAeGgAwIBAgIBATALBgkqhkiG9w0BAQUwJDEQMA4GA1UEChMHQWNtZSBD"
    "bzEQMA4GA1UEAxMHUm9vdCBDQTAeFw0xMzAxMDExMDAwMDBaFw0yMzEyMzExMDAw"
    "MDBaMCQxEDAOBgNVBAoTB0FjbWUgQ28xEDAOBgNVBAMTB1Jvb3QgQ0EwggEiMA0G"
    "CSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDkK5HHfasytzjhOK7IuQsVHGR2R4Rd"
    "y+fnDDB3hGqOdZVCsnjIiBCsmEeXONE6f4YNIPEdcISine0aKK9eQ90xo7u4XMSD"
    "ebiDmuepYwRZk7YmZy3d5i275BPr1RcL3mNGdm8QBUCwFvzq9Jcc1tz+N3LVQN/j"
    "tNWsz8mufCFJAR5+1MHhKhEBtHA6MT2aM7d/IPKL51SOBvJLX/DiuY9kH1C9s6Ws"
    "aURCbBLpEZ10tEl34w+LnJRTFwwjumH6cD2Tja1f3U8yhFsHUORYxwBFgh8hFEy/"
    "Q5J2+yQJM99Yjb6H7rVU5NMy9rEtaXSGrR9XfpsFEXS1xGismoB0ejSJAgMBAAGj"
    "ODA2MA4GA1UdDwEB/wQEAwIABDATBgNVHSUEDDAKBggrBgEFBQcDATAPBgNVHRMB"
    "Af8EBTADAQH/MAsGCSqGSIb3DQEBBQOCAQEAls3bRm5b3vrx0xzg/kdnKlnV+MQL"
    "JRQLBo2CZ/SpNuhTvOtAUQWKQgnnSKN8Qm3BNwZJz1iH0A7HnksONHL4ZWWyxGjK"
    "oxTpEVzaeE50gEPcuLbOqAyij1JZieBaAeXpuEsxkSW/feF8huk2wVsQ5SzMb5nE"
    "ZnkwQR8L+UvqHopFczx5ISDIgMT06U+FaXwuYYA6T1uSvpcSdZ5DCQG2s6HBXy2G"
    "vtFsVe4n+L86vPuyQopvUaDTRlT2HnNCKpVe67xAa3G/kJRi9JAXguUeM9v0UBHl"
    "VRAJahGpHtQHYFj3FrG9iyn2PWGtc9qu4+RuWUZ/wPv6vm18MZSGLrMpew==";

const char kExampleCertificateRootHash[] =
    "\x96\x99\xba\x1e\x7a\xf3\x45\x08\xcc\xcf\xf8\x77\xcd\xa1\xa8\x37"
    "\x70\x15\x0b\x3e\x79\xa4\x8f\x7f\xc8\xc3\x6e\x28\x5b\x3c\x3b\x65";

const char kOtherCertificateHash[] =
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01";

#ifdef OS_WIN
const char kRevocationInfoDownloadTestRoot[] =
    "MIIB0TCCAToCCQDUjpfMAN6R4jANBgkqhkiG9w0BAQsFADAtMQswCQYDVQQGEwJQ"
    "TDEOMAwGA1UECAwFU3RhdGUxDjAMBgNVBAoMBU9wZXJhMB4XDTE2MDEyMTEyMTUz"
    "NVoXDTE2MDIyMDEyMTUzNVowLTELMAkGA1UEBhMCUEwxDjAMBgNVBAgMBVN0YXRl"
    "MQ4wDAYDVQQKDAVPcGVyYTCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEAzRFM"
    "OQn6eein8/sizNhZBWFd7iIfPWXQABG5Gn5bK0gv31BsSbRhS2dGy9QNRhkGODhS"
    "0kC78dt09jdFpm4E6jUOusC7EeJsyqE/jVsP7xs27Crjt6dsuoQmjSgEBvAdl5qe"
    "ZeX3uSdh0sSI9boUgoYCVubdUtqQwaDGH3yRn6ECAwEAATANBgkqhkiG9w0BAQsF"
    "AAOBgQCzPMPcHWPWrd8aB4CzjSVcmXnI8JrceaokHttn0Yrt2Ur8G93j24K9v7mr"
    "kjHqVEQIh8NECEby275EdL7EYp0pjQTDWD/lJpTno4LhKWAjDPLr6BIeCjBFaMC5"
    "xmeN9T0wnZ+h3fgEQqU1Ep0u8Cn+qmIb2UGqLlK1WNEpxv9qyg==";

const char kRevocationInfoDownloadTestLeaf[] =
    "MIIDEzCCAnygAwIBAgIBADANBgkqhkiG9w0BAQsFADAtMQswCQYDVQQGEwJQTDEO"
    "MAwGA1UECAwFU3RhdGUxDjAMBgNVBAoMBU9wZXJhMB4XDTE2MDEyMTE0MDgyNFoX"
    "DTI2MDExODE0MDgyNFowTDELMAkGA1UEBhMCUEwxDjAMBgNVBAgMBVN0YXRlMQ4w"
    "DAYDVQQKDAVPcGVyYTEdMBsGCSqGSIb3DQEJARYOdGVzdEBvcGVyYS5jb20wgZ8w"
    "DQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBALWXAQps2IbPhv/8mclrDzv4ujqGeJrq"
    "kD3onV/XBU8uoWYHZvCp9F4j2NDFCo9nJEH+LPT8Cbdc4dtVvRSCxcJqg/Yd2xBe"
    "sLzmd1ZX+9A5dA9hj9D9Awt68NHFW+eJ2NNCmHTubPsN8TkzF7TwyVdrfWenwsa0"
    "ZinBHtZp4WznAgMBAAGjggEiMIIBHjAMBgNVHRMEBTADAQH/MCwGCWCGSAGG+EIB"
    "DQQfFh1PcGVuU1NMIEdlbmVyYXRlZCBDZXJ0aWZpY2F0ZTAdBgNVHQ4EFgQUrAqo"
    "8EIotQPs1S9bRyy4HJhIwBwwRwYDVR0jBEAwPqExpC8wLTELMAkGA1UEBhMCUEwx"
    "DjAMBgNVBAgMBVN0YXRlMQ4wDAYDVQQKDAVPcGVyYYIJANSOl8wA3pHiMBkGA1Ud"
    "EQQSMBCBDnRlc3RAb3BlcmEuY29tMDEGCCsGAQUFBwEBBCUwIzAhBggrBgEFBQcw"
    "AYYVaHR0cDovL2xvY2FsaG9zdC9vY3NwMCoGA1UdHwQjMCEwH6AdoBuGGWh0dHA6"
    "Ly9sb2NhbGhvc3QvdGVzdC5jcmwwDQYJKoZIhvcNAQELBQADgYEAfLS/Gwt+n8GW"
    "HwINIiFDENCQKUBDhViwoOzjPcWqJ8V5A8SU6N9kTAQW+26GP56Ck++UUvUIyh2a"
    "MjyP9GSSUG9AMlOnROZe1dKkgBhHF+RspCQdwvGxJEOHxgjeJd7wSYGt8qmL6Qtf"
    "WywSt/veYL3+GvBWIl69HszOAWHJ8jo=";

const char kRevocationInfoDownloadTestLeafNoOcsp[] =
    "MIIC3jCCAkegAwIBAgIBATANBgkqhkiG9w0BAQsFADAtMQswCQYDVQQGEwJQTDEO"
    "MAwGA1UECAwFU3RhdGUxDjAMBgNVBAoMBU9wZXJhMB4XDTE2MDEyMTE0MTAxNloX"
    "DTI2MDExODE0MTAxNlowTDELMAkGA1UEBhMCUEwxDjAMBgNVBAgMBVN0YXRlMQ4w"
    "DAYDVQQKDAVPcGVyYTEdMBsGCSqGSIb3DQEJARYOdGVzdEBvcGVyYS5jb20wgZ8w"
    "DQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBALWXAQps2IbPhv/8mclrDzv4ujqGeJrq"
    "kD3onV/XBU8uoWYHZvCp9F4j2NDFCo9nJEH+LPT8Cbdc4dtVvRSCxcJqg/Yd2xBe"
    "sLzmd1ZX+9A5dA9hj9D9Awt68NHFW+eJ2NNCmHTubPsN8TkzF7TwyVdrfWenwsa0"
    "ZinBHtZp4WznAgMBAAGjge4wgeswDAYDVR0TBAUwAwEB/zAsBglghkgBhvhCAQ0E"
    "HxYdT3BlblNTTCBHZW5lcmF0ZWQgQ2VydGlmaWNhdGUwHQYDVR0OBBYEFKwKqPBC"
    "KLUD7NUvW0csuByYSMAcMEcGA1UdIwRAMD6hMaQvMC0xCzAJBgNVBAYTAlBMMQ4w"
    "DAYDVQQIDAVTdGF0ZTEOMAwGA1UECgwFT3BlcmGCCQDUjpfMAN6R4jAZBgNVHREE"
    "EjAQgQ50ZXN0QG9wZXJhLmNvbTAqBgNVHR8EIzAhMB+gHaAbhhlodHRwOi8vbG9j"
    "YWxob3N0L3Rlc3QuY3JsMA0GCSqGSIb3DQEBCwUAA4GBAJM8MytzSONFGluI9iQr"
    "LCWDhftGRv0DGriO9BspMPqTXJLfL9H05a+YwaEEwdNXCE7fg+c/z4XSS1E/gn7s"
    "aODjRb7iu0mwzplYaOtNo0/7gjNS+/dSZmYpEeFK9j3aC3pFVHUf3PuLWVxUIayP"
    "/1QKEOYuYcizMT5fjpSTviyb";

const char kOcspCheckUrl[] =
    "http://localhost/ocsp/MEIwQDA%2BMDwwOjAJBg"
    "UrDgMCGgUABBRE4waMtgL8Wsh6qnKUwsVZ0rYFLgQUp792H837Q0Z28Gn1cI0BaDG88qUCAQA%"
    "3D";

const char kCrlCheckUrl[] = "http://localhost/test.crl";
#endif

const char kCrlSetHeader[] =
    "{"
    "\"Version\":0,"
    "\"ContentType\":\"CRLSet\","
    "\"Sequence\":778,\"DeltaFrom\":0,"
    "\"NumParents\":47,"
    "\"BlockedSPKIs\":"
    "  [\"GvVsmP8EPvkr6/9UzrtN1nolupVsgX8+bdPB5S61hME=\","
    "  \"PtvZrOY5uhotStBHGHEf2iPoWbL79dE31CQEXnkZ37k=\"],"
    "\"NotAfter\":1359359232"
    "}";

const char kCrlSetHeaderBlockExampleCa[] =
    "{"
    "\"Version\":0,"
    "\"ContentType\":\"CRLSet\","
    "\"Sequence\":778,\"DeltaFrom\":0,"
    "\"NumParents\":47,"
    "\"BlockedSPKIs\":"
    // the SPKI of the blocked root.
    "  [\"5Dqj25gxYQXdV23GL3Emut30mD5iIvj55Bhid3nbmzE=\","
    "  \"PtvZrOY5uhotStBHGHEf2iPoWbL79dE31CQEXnkZ37k=\"],"
    "\"NotAfter\":1359359232"
    "}";

int CreateCRLSet(const char* crl_data,
                 scoped_refptr<net::CRLSet>* out_crl_set) {
  std::string crl_list_data;

  int16_t header_length = base::checked_cast<int16_t>(strlen(crl_data));

  // Serialize header_length into two bytes in little endian format.
  // CRLSet::Parse assures that CRLSet only compiles on little endian hardware.
  // Read documentation in net/base/crl_set.cc for CRLSet file format.
  crl_list_data.assign(reinterpret_cast<const char*>(&header_length), 2);

  base::StringPiece crl_set_header_string(crl_data);
  crl_set_header_string.AppendToString(&crl_list_data);

  return net::CRLSetStorage::Parse(crl_list_data, out_crl_set);
}

class ScopedCrlSetOverride {
 public:
  explicit ScopedCrlSetOverride(scoped_refptr<net::CRLSet> new_crl_set)
      : original_crl_set_(
            OperaCertVerifierDelegate::GetCertificateRevocationList()) {
    OperaCertVerifierDelegate::SetCertificateRevocationList(new_crl_set);
  }

  ~ScopedCrlSetOverride() {
    OperaCertVerifierDelegate::SetCertificateRevocationList(original_crl_set_);
  }

 private:
  scoped_refptr<net::CRLSet> original_crl_set_;
};

class CertificateTest : public ::testing::Test {
 public:
  CertificateTest()
      : ui_thread_(content::BrowserThread::IO, &loop_),
        cert_verifier_(base::WrapUnique(new OperaCertVerifierDelegate(
            net::CertVerifier::CreateDefault()))) {
    std::string der_example_certificate;
    std::string der_example_certificate_intermediate;
    std::string der_example_certificate_root;

    EXPECT_TRUE(base::Base64Decode(kExampleCertificateIntermediate,
                                   &der_example_certificate_intermediate));
    EXPECT_TRUE(
        base::Base64Decode(kExampleCertificate, &der_example_certificate));
    EXPECT_TRUE(base::Base64Decode(kExampleCertificateRoot,
                                   &der_example_certificate_root));

    std::vector<base::StringPiece> der_encoded_cert_chain;
    der_encoded_cert_chain.push_back(der_example_certificate);
    der_encoded_cert_chain.push_back(der_example_certificate_intermediate);
    cert_ = X509Certificate::CreateFromDERCertChain(der_encoded_cert_chain);
    CHECK(cert_);

    // Add the root certificate and mark it as trusted.
    std::vector<base::StringPiece> der_encoded_cert_chain_root;
    net::TestRootCerts* test_roots = net::TestRootCerts::GetInstance();
    der_encoded_cert_chain_root.push_back(der_example_certificate_root);
    test_roots->Add(
        X509Certificate::CreateFromDERCertChain(der_encoded_cert_chain_root).get());
  }

  int VerifyCertificate() {
    std::unique_ptr<CertVerifier::Request> request;
    net::CertVerifyResult verify_result;
    net::TestCompletionCallback completion_callback;
    const auto rv = cert_verifier_->Verify(
        cert_.get(), kExampleHostname, std::string(), 0, NULL, &verify_result,
        completion_callback.callback(), &request, BoundNetLog());
    return completion_callback.GetResult(rv);
  }

 protected:
  base::MessageLoop loop_;
  content::TestBrowserThread ui_thread_;
  std::unique_ptr<OperaCertVerifierDelegate> cert_verifier_;

  scoped_refptr<net::X509Certificate> cert_;
};


using CertificateBlacklistTest = CertificateTest;

TEST_F(CertificateBlacklistTest, VerifyCertificateCARevoked) {
  scoped_refptr<net::CRLSet> crl_set_ca_revoked;
  EXPECT_TRUE(
      CreateCRLSet(kCrlSetHeaderBlockExampleCa, &crl_set_ca_revoked));

  ScopedCrlSetOverride crl_set_override(crl_set_ca_revoked);
  EXPECT_EQ(net::ERR_CERT_REVOKED, VerifyCertificate());
}

TEST_F(CertificateBlacklistTest, VerifyCertificateCAValid) {
  scoped_refptr<net::CRLSet> crl_set;
  EXPECT_TRUE(CreateCRLSet(kCrlSetHeader, &crl_set));

  ScopedCrlSetOverride crl_set_override(crl_set);
  EXPECT_EQ(net::OK, VerifyCertificate());
}


using CertificatePinningTest = CertificateTest;

TEST_F(CertificatePinningTest, NoPinForHost) {
  cert_verifier_->AddPinForHost(kExampleHostnameDomain, kOtherCertificateHash);
  cert_verifier_->AddPinForHost(std::string("sub.") + kExampleHostname,
                                kOtherCertificateHash);
  cert_verifier_->AddPinForHost(
      std::string("mismatch.*.") + kExampleHostnameDomain,
      kOtherCertificateHash);
  EXPECT_EQ(net::OK, VerifyCertificate());
}

TEST_F(CertificatePinningTest, PinVerified) {
  cert_verifier_->AddPinForHost(kExampleHostname, kExampleCertificateRootHash);
  EXPECT_EQ(net::OK, VerifyCertificate());
}

TEST_F(CertificatePinningTest, PinVerified_HostnamePatternMatched) {
  cert_verifier_->AddPinForHost(std::string("*.") + kExampleHostnameDomain,
                                kExampleCertificateRootHash);
  EXPECT_EQ(net::OK, VerifyCertificate());
}

TEST_F(CertificatePinningTest, PinNotVerified) {
  cert_verifier_->AddPinForHost(kExampleHostname, kOtherCertificateHash);
  EXPECT_EQ(net::ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN, VerifyCertificate());
}

TEST_F(CertificatePinningTest, PinNotVerified_HostnamePatternMatched) {
  cert_verifier_->AddPinForHost(std::string("*.") + kExampleHostnameDomain,
                                kOtherCertificateHash);
  EXPECT_EQ(net::ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN, VerifyCertificate());
}

#ifdef OS_WIN
class CertificateRevocationInfoDownloadTest : public ::testing::Test {
 public:
  CertificateRevocationInfoDownloadTest()
      : loop_(base::MessageLoop::TYPE_IO),
        ui_thread_(content::BrowserThread::IO, &loop_) {
    std::string der_test_certificate_leaf;
    std::string der_test_certificate_leaf_noocsp;
    std::string der_test_certificate_root;

    EXPECT_TRUE(base::Base64Decode(kRevocationInfoDownloadTestLeaf,
                                   &der_test_certificate_leaf));
    EXPECT_TRUE(base::Base64Decode(kRevocationInfoDownloadTestLeafNoOcsp,
                                   &der_test_certificate_leaf_noocsp));
    EXPECT_TRUE(base::Base64Decode(kRevocationInfoDownloadTestRoot,
                                   &der_test_certificate_root));

    std::vector<base::StringPiece> der_encoded_cert_chain;
    der_encoded_cert_chain.push_back(der_test_certificate_leaf);
    cert_ = X509Certificate::CreateFromDERCertChain(der_encoded_cert_chain);
    EXPECT_TRUE(cert_);
    std::vector<base::StringPiece> der_encoded_cert_chain_noocsp;
    der_encoded_cert_chain_noocsp.push_back(der_test_certificate_leaf_noocsp);
    cert_no_ocsp_ =
        X509Certificate::CreateFromDERCertChain(der_encoded_cert_chain_noocsp);
    EXPECT_TRUE(cert_no_ocsp_);

    // Add the root certificate and mark it as trusted.
    std::vector<base::StringPiece> der_encoded_cert_chain_root;
    net::TestRootCerts* test_roots = net::TestRootCerts::GetInstance();
    der_encoded_cert_chain_root.push_back(der_test_certificate_root);
    test_roots->Add(
        X509Certificate::CreateFromDERCertChain(der_encoded_cert_chain_root).get());

    request_context_getter_ =
        new net::TestURLRequestContextGetter(loop_.task_runner());
    EXPECT_TRUE(temp_.CreateUniqueTempDir());
    response_file_path_ = temp_.path().AppendASCII("response");
    EXPECT_EQ(base::WriteFile(response_file_path_, "0", 1), 1);
    interceptor_.reset(new net::LocalHostTestURLRequestInterceptor(
        loop_.task_runner(), loop_.task_runner()));
    OperaCertVerifierDelegate* opera_verifier =
        new OperaCertVerifierDelegate(net::CertVerifier::CreateDefault());
    opera_verifier->CreateFreedomRevocationFetcher(request_context_getter_);
    cert_verifier_.reset(opera_verifier);
    loop_.RunUntilIdle();
  }

  ~CertificateRevocationInfoDownloadTest() {
    interceptor_.reset(nullptr);
    // Let the interceptor to unregister
    loop_.RunUntilIdle();
  }

  int StartDownload(net::X509Certificate* cert,
                    const GURL& url,
                    net::CertVerifyResult* verify_result,
                    std::unique_ptr<CertVerifier::Request>* out_req,
                    const GURL* interception_url) {
    interceptor_->SetResponse(interception_url ? *interception_url : url,
                              response_file_path_);
    loop_.RunUntilIdle();
    return cert_verifier_->Verify(cert, "test.example.com", std::string(), 0,
                                  NULL, verify_result, callback_.callback(),
                                  out_req, BoundNetLog());
  }

  int StartDownload(net::X509Certificate* cert,
                    const GURL& url,
                    net::CertVerifyResult* verify_result,
                    std::unique_ptr<CertVerifier::Request>* out_req) {
    return StartDownload(cert, url, verify_result, out_req, nullptr);
  }

  void TestDownload(net::X509Certificate* cert, const GURL& url) {
    std::unique_ptr<CertVerifier::Request> request;
    net::CertVerifyResult verify_result;
    int rv = StartDownload(cert, url, &verify_result, &request);
    EXPECT_EQ(rv, net::ERR_IO_PENDING);
    EXPECT_EQ(callback_.GetResult(rv), net::ERR_CERT_COMMON_NAME_INVALID);
    EXPECT_EQ(2 /* 1 for load from cache and 1 for load bypassing the cache. */,
              interceptor_->GetHitCount());
  }

 protected:
  base::MessageLoop loop_;
  content::TestBrowserThread ui_thread_;

  scoped_refptr<net::X509Certificate> cert_;
  scoped_refptr<net::X509Certificate> cert_no_ocsp_;
  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
  std::unique_ptr<net::LocalHostTestURLRequestInterceptor> interceptor_;
  std::unique_ptr<net::CertVerifier> cert_verifier_;
  base::ScopedTempDir temp_;
  base::FilePath response_file_path_;
  net::TestCompletionCallback callback_;
};

TEST_F(CertificateRevocationInfoDownloadTest, RevocationDownloadOCSP) {
  TestDownload(cert_.get(), GURL(kOcspCheckUrl));
}

TEST_F(CertificateRevocationInfoDownloadTest, RevocationDownloadCRL) {
  TestDownload(cert_no_ocsp_.get(), GURL(kCrlCheckUrl));
}

TEST_F(CertificateRevocationInfoDownloadTest, RevocationDownloadCancellation) {
  std::unique_ptr<CertVerifier::Request> request;
  net::CertVerifyResult verify_result;
  int rv = StartDownload(cert_.get(), GURL(kOcspCheckUrl), &verify_result,
                         &request);
  EXPECT_EQ(rv, net::ERR_IO_PENDING);
  base::MessageLoop::current()->DeleteSoon(FROM_HERE, request.release());
  base::MessageLoop::current()->PostDelayedTask(
      FROM_HERE, base::Bind(callback_.callback(), net::ERR_TIMED_OUT),
      base::TimeDelta::FromMilliseconds(1000));
  EXPECT_EQ(callback_.GetResult(rv), net::ERR_TIMED_OUT);
  EXPECT_EQ(1, interceptor_->GetHitCount());
}

TEST_F(CertificateRevocationInfoDownloadTest,
       RevocationDownloadOCSPFallsbackToCRLonError) {
  std::unique_ptr<CertVerifier::Request> request;
  net::CertVerifyResult verify_result;
  GURL crl_url = GURL(kCrlCheckUrl);
  // Used a method allowing to set intercept URL to a different URL that
  // the download one.
  // The test intercepts CLR url only - it will result in error for OCSP one
  // because TestURLRequestContext produced by TestURLRequestContextGetter used
  // in these tests is not allowed to make network connections.
  // Since CRL url is intercepted and OCSP fails hit count means CRL was used.
  int rv = StartDownload(cert_.get(), GURL(kOcspCheckUrl), &verify_result,
                         &request, &crl_url);
  EXPECT_EQ(rv, net::ERR_IO_PENDING);
  EXPECT_EQ(callback_.GetResult(rv), net::ERR_CERT_COMMON_NAME_INVALID);
  // Hit count should be 2 because the fallback url is first tried to be loaded
  // form the cache.
  EXPECT_EQ(2, interceptor_->GetHitCount());
}
#endif

}  // namespace
}  // namespace opera
