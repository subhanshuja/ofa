// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA.

#include "common/net/certificate_util.h"

#include <Cryptuiapi.h>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "Cryptnet.lib")

#include "base/base64.h"
#include "base/memory/free_deleter.h"
#include "base/sha1.h"
#include "crypto/capi_util.h"
#include "crypto/scoped_capi_types.h"
#include "net/base/escape.h"
#include "net/cert/ev_root_ca_metadata.h"
#include "url/gurl.h"

namespace opera {
namespace cert_util {

namespace {

struct FreeCertChainContextFunctor {
  void operator()(PCCERT_CHAIN_CONTEXT chain_context) const {
    if (chain_context)
      CertFreeCertificateChain(chain_context);
  }
};

struct FreeCertContextFunctor {
  void operator()(PCCERT_CONTEXT context) const {
    if (context)
      CertFreeCertificateContext(context);
  }
};

struct FreeCRLContextFunctor {
  void operator()(PCCRL_CONTEXT crl_context) const {
    if (crl_context)
      CertFreeCRLContext(crl_context);
  }
};

struct FreeChainEngineFunctor {
  void operator()(HCERTCHAINENGINE engine) const {
    if (engine)
      CertFreeCertificateChainEngine(engine);
  }
};

typedef std::unique_ptr<const CERT_CHAIN_CONTEXT, FreeCertChainContextFunctor>
    ScopedPCCERT_CHAIN_CONTEXT;

typedef std::unique_ptr<const CERT_CONTEXT, FreeCertContextFunctor>
    ScopedPCCERT_CONTEXT;

typedef std::unique_ptr<const CRL_CONTEXT, FreeCRLContextFunctor>
    ScopedPCCRL_CONTEXT;

typedef crypto::ScopedCAPIHandle<HCERTCHAINENGINE, FreeChainEngineFunctor>
    ScopedHCERTCHAINENGINE;

template <typename EXTENSION_TYPE>
bool GetExtension(
    PCCERT_CONTEXT cert_handle,
    LPCSTR oid,
    std::unique_ptr<EXTENSION_TYPE, base::FreeDeleter>* out_extension) {
  PCERT_EXTENSION cert_extension =
      CertFindExtension(oid, cert_handle->pCertInfo->cExtension,
                        cert_handle->pCertInfo->rgExtension);
  if (!cert_extension)
    return false;

  CRYPT_DECODE_PARA decode_para;
  decode_para.cbSize = sizeof(decode_para);
  decode_para.pfnAlloc = crypto::CryptAlloc;
  decode_para.pfnFree = crypto::CryptFree;
  EXTENSION_TYPE* extension = nullptr;
  DWORD size = 0;
  BOOL rv;
  rv = CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, oid,
                           cert_extension->Value.pbData,
                           cert_extension->Value.cbData,
                           CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG,
                           &decode_para, &extension, &size);

  if (rv)
    out_extension->reset(extension);

  return rv == TRUE;
}

PCCERT_CHAIN_CONTEXT GetCertChain(PCCERT_CONTEXT cert_handle, bool verify_ev) {
  if (!cert_handle)
    return NULL;

  // Build certificate chain.
  // Partially taken from net/cert/cert_verifier_proc_win.cc
  CERT_CHAIN_PARA chain_para;
  memset(&chain_para, 0, sizeof(chain_para));
  chain_para.cbSize = sizeof(chain_para);
  static const LPCSTR usage[] = {szOID_PKIX_KP_SERVER_AUTH,
                                 szOID_SERVER_GATED_CRYPTO, szOID_SGC_NETSCAPE};
  chain_para.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
  chain_para.RequestedUsage.Usage.cUsageIdentifier = arraysize(usage);
  chain_para.RequestedUsage.Usage.rgpszUsageIdentifier =
      const_cast<LPSTR*>(usage);
  LPSTR ev_policy_oid = NULL;
  if (verify_ev) {
    std::unique_ptr<CERT_POLICIES_INFO, base::FreeDeleter> scoped_policies_info;
    if (!GetExtension(cert_handle, szOID_CERT_POLICIES, &scoped_policies_info))
      return NULL;

    if (scoped_policies_info.get()) {
      net::EVRootCAMetadata* metadata = net::EVRootCAMetadata::GetInstance();
      for (DWORD i = 0; i < scoped_policies_info->cPolicyInfo; ++i) {
        LPSTR policy_oid =
            scoped_policies_info->rgPolicyInfo[i].pszPolicyIdentifier;
        if (metadata->IsEVPolicyOID(policy_oid)) {
          ev_policy_oid = policy_oid;
          chain_para.RequestedIssuancePolicy.dwType = USAGE_MATCH_TYPE_AND;
          chain_para.RequestedIssuancePolicy.Usage.cUsageIdentifier = 1;
          chain_para.RequestedIssuancePolicy.Usage.rgpszUsageIdentifier =
              &ev_policy_oid;
          break;
        }
      }
    }
  }

  DWORD chain_flags = CERT_CHAIN_CACHE_END_CERT |
                      CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT |
                      CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY;
  PCCERT_CHAIN_CONTEXT chain_context;
  CertGetCertificateChain(NULL, cert_handle,
                          NULL,  // current system time
                          cert_handle->hCertStore, &chain_para, chain_flags,
                          NULL,  // reserved
                          &chain_context);

  return chain_context;
}

CertificateChain Translate(HCERTSTORE store,
                           PCCERT_CHAIN_CONTEXT chain_context) {
  CertificateChain chain;
  const PCERT_SIMPLE_CHAIN first_chain = chain_context->rgpChain[0];
  const PCERT_CHAIN_ELEMENT* element = first_chain->rgpElement;
  const int num_elements = first_chain->cElement;
  for (int i = 0; i < num_elements; ++i) {
    // Make sure all the certificates are in a proper store
    // with a proper life span.
    PCCERT_CONTEXT cert_in_scoped_store = nullptr;
    // CERT_STORE_ADD_USE_EXISTING causes the certificate is added
    // if it's not present there already. Otherwise its handle is duplicated.
    BOOL ok = CertAddCertificateContextToStore(store, element[i]->pCertContext,
                                               CERT_STORE_ADD_USE_EXISTING,
                                               &cert_in_scoped_store);
    if (ok && cert_in_scoped_store)
      chain.push_back(cert_in_scoped_store);
  }

  return chain;
}

bool IsTimeValid(const FILETIME* this_update, const FILETIME* next_update) {
  SYSTEMTIME system_time;
  GetSystemTime(&system_time);
  FILETIME system_time_as_file_time;
  if (SystemTimeToFileTime(&system_time, &system_time_as_file_time)) {
    return (CompareFileTime(&system_time_as_file_time, this_update) >= 0) &&
           (CompareFileTime(&system_time_as_file_time, next_update) < 0);
  }

  return false;
}

bool IsTimeValidCRL(PCCRL_CONTEXT crl) {
  return IsTimeValid(&crl->pCrlInfo->ThisUpdate, &crl->pCrlInfo->NextUpdate);
}

bool IsTimeValidOCSPResponse(BYTE* data, DWORD size) {
  CRYPT_DECODE_PARA decode_para;
  decode_para.cbSize = sizeof(decode_para);
  decode_para.pfnAlloc = crypto::CryptAlloc;
  decode_para.pfnFree = crypto::CryptFree;
  OCSP_RESPONSE_INFO* ocsp_response = nullptr;
  BOOL rv;
  rv = CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                           OCSP_RESPONSE, data, size,
                           CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG,
                           &decode_para, &ocsp_response, &size);
  if (!rv)
    return false;

  std::unique_ptr<OCSP_RESPONSE_INFO, base::FreeDeleter> scoped_response(
      ocsp_response);
  OCSP_BASIC_SIGNED_RESPONSE_INFO* signed_response = nullptr;
  rv = CryptDecodeObjectEx(
      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, OCSP_BASIC_SIGNED_RESPONSE,
      ocsp_response->Value.pbData, ocsp_response->Value.cbData,
      CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG, &decode_para,
      &signed_response, &size);
  if (!rv)
    return false;

  std::unique_ptr<OCSP_BASIC_SIGNED_RESPONSE_INFO, base::FreeDeleter>
      scoped_signed_response(signed_response);
  OCSP_BASIC_RESPONSE_INFO* basic_response = nullptr;
  rv = CryptDecodeObjectEx(
      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, OCSP_BASIC_RESPONSE,
      signed_response->ToBeSigned.pbData, signed_response->ToBeSigned.cbData,
      CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG, &decode_para,
      &basic_response, &size);
  if (!rv)
    return false;

  std::unique_ptr<OCSP_BASIC_RESPONSE_INFO, base::FreeDeleter> scoped_basic_response(
      basic_response);

  for (DWORD i = 0; i < basic_response->cResponseEntry; ++i) {
    if (!IsTimeValid(&basic_response->rgResponseEntry[i].ThisUpdate,
                     &basic_response->rgResponseEntry[i].NextUpdate))
      return false;
  }

  return true;
}

}  // namespace

CertificateRevocationCheckChains GetCertificateRevocationCheckChains(
    const net::X509Certificate& cert,
    bool verify_is_ev) {
  PCCERT_CONTEXT cert_handle = cert.os_cert_handle();
  if (!cert_handle)
    return CertificateRevocationCheckChains();

  ScopedPCCERT_CONTEXT cert_list(cert.CreateOSCertChainForCert());
  if (!cert_list)
    return CertificateRevocationCheckChains();

  PCCERT_CHAIN_CONTEXT chain_context =
      GetCertChain(cert_list.get(), verify_is_ev);
  if (!chain_context)
    return CertificateRevocationCheckChains();

  ScopedPCCERT_CHAIN_CONTEXT scoped_chain_context(chain_context);

  // Check if all certificates from the input chain are in the output chain.
  // If not add chains for them to the output. This is because CRYPTO may check
  // revocation for them internally so we'd better do it too to have results for
  // CRYPTO to reuse.
  PCCERT_CONTEXT prev = NULL;
  std::vector<PCCERT_CONTEXT> not_in_out_chain;
  for (PCCERT_CONTEXT iter; (iter = CertEnumCertificatesInStore(
                                 cert_list->hCertStore, prev)) != NULL;
       prev = iter) {
    const PCERT_SIMPLE_CHAIN first_chain = chain_context->rgpChain[0];
    const PCERT_CHAIN_ELEMENT* element = first_chain->rgpElement;
    const int num_elements = first_chain->cElement;
    bool found = false;
    for (int i = num_elements - 1; i >= 0; --i) {
      if (net::X509Certificate::IsSameOSCert(iter, element[i]->pCertContext)) {
        found = true;
        break;
      }
    }

    if (!found && !net::X509Certificate::IsSelfSigned(iter))
      not_in_out_chain.push_back(iter);
  }

  CertificateRevocationCheckChains result;
  for (auto not_in_out_chain_elem : not_in_out_chain) {
    PCCERT_CHAIN_CONTEXT aux_chain_context =
        GetCertChain(not_in_out_chain_elem, verify_is_ev);
    ScopedPCCERT_CHAIN_CONTEXT scoped_aux_chain_context(aux_chain_context);
    if (aux_chain_context)
      // Make sure all the handles are in the |cert_list->hCertStore| store
      // because this one is cached and closed only after all memmbers' handles
      // have been closed. Same below.
      result.push_back(Translate(cert_list->hCertStore, aux_chain_context));
  }

  result.push_back(Translate(cert_list->hCertStore, chain_context));
  return result;
}

std::vector<GURL> GetOCSPURLs(
    const net::X509Certificate::OSCertHandle& subject,
    const net::X509Certificate::OSCertHandle& issuer) {
  DWORD urls_num = 0;
  if (!CryptGetObjectUrl(
          URL_OID_CERTIFICATE_OCSP,
          reinterpret_cast<LPVOID>(const_cast<PCERT_CONTEXT>(subject)),
          CRYPT_GET_URL_FROM_PROPERTY | CRYPT_GET_URL_FROM_EXTENSION |
              CRYPT_GET_URL_FROM_AUTH_ATTRIBUTE |
              CRYPT_GET_URL_FROM_UNAUTH_ATTRIBUTE,
          NULL, &urls_num, NULL, NULL, 0))
    return std::vector<GURL>();

  CRYPT_URL_ARRAY* urls =
      reinterpret_cast<CRYPT_URL_ARRAY*>(crypto::CryptAlloc(urls_num));
  std::unique_ptr<CRYPT_URL_ARRAY, base::FreeDeleter> scoped_urls(urls);
  CryptGetObjectUrl(
      URL_OID_CERTIFICATE_OCSP,
      reinterpret_cast<LPVOID>(const_cast<PCERT_CONTEXT>(subject)),
      CRYPT_GET_URL_FROM_PROPERTY | CRYPT_GET_URL_FROM_EXTENSION |
          CRYPT_GET_URL_FROM_AUTH_ATTRIBUTE |
          CRYPT_GET_URL_FROM_UNAUTH_ATTRIBUTE,
      urls, &urls_num, NULL, NULL, 0);

  std::vector<GURL> urls_from_cert;
  for (DWORD i = 0; i < urls->cUrl; ++i) {
    base::string16 url_wide_str(urls->rgwszUrl[i]);
    urls_from_cert.push_back(GURL(url_wide_str));
  }

  CRYPT_ALGORITHM_IDENTIFIER alg;
  ZeroMemory(&alg, sizeof(alg));
  alg.pszObjId = const_cast<char*>(szOID_OIWSEC_sha1);
  OCSP_CERT_ID id;
  ZeroMemory(&id, sizeof(id));
  id.HashAlgorithm = alg;
  id.SerialNumber = subject->pCertInfo->SerialNumber;
  net::HashValue issuer_hash(net::HASH_VALUE_SHA1);
  base::SHA1HashBytes(subject->pCertInfo->Issuer.pbData,
                      subject->pCertInfo->Issuer.cbData, issuer_hash.data());
  CRYPT_HASH_BLOB issuer_hash_blob;
  issuer_hash_blob.pbData = issuer_hash.data();
  issuer_hash_blob.cbData = issuer_hash.size();
  id.IssuerNameHash = issuer_hash_blob;

  net::HashValue issuer_key_hash(net::HASH_VALUE_SHA1);
  base::SHA1HashBytes(issuer->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
                      issuer->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData,
                      issuer_key_hash.data());
  CRYPT_HASH_BLOB key_hash_blob;
  key_hash_blob.pbData = issuer_key_hash.data();
  key_hash_blob.cbData = issuer_key_hash.size();
  id.IssuerKeyHash = key_hash_blob;

  OCSP_REQUEST_ENTRY entry[1];
  ZeroMemory(entry, sizeof(entry));
  entry[0].CertId = id;
  OCSP_REQUEST_INFO request;
  ZeroMemory(&request, sizeof(request));
  request.cRequestEntry = 1;
  request.rgRequestEntry = entry;
  request.dwVersion = OCSP_REQUEST_V1;
  CRYPT_ENCODE_PARA encode_para;
  encode_para.cbSize = sizeof(CRYPT_ENCODE_PARA);
  encode_para.pfnAlloc = crypto::CryptAlloc;
  encode_para.pfnFree = crypto::CryptFree;
  BYTE* encoded_request = 0;
  DWORD encoded_request_size = 0;

  BOOL done = CryptEncodeObjectEx(
      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
      OCSP_REQUEST,  // Not supported for all win versions e.g. not for xp
      &request, CRYPT_ENCODE_ALLOC_FLAG, &encode_para, &encoded_request,
      &encoded_request_size);
  if (!done) {
    VLOG(ERROR) << "CryptEncodeObjectEx failed with error: " << GetLastError();
    return std::vector<GURL>();
  }
  std::unique_ptr<BYTE, base::FreeDeleter> scoped_encoded_request(encoded_request);
  OCSP_SIGNED_REQUEST_INFO signed_request;
  signed_request.ToBeSigned.pbData = encoded_request;
  signed_request.ToBeSigned.cbData = encoded_request_size;
  signed_request.pOptionalSignatureInfo = 0;

  BYTE* encoded_signed_request = 0;
  DWORD encoded_signed_request_size = 0;
  done = CryptEncodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                             OCSP_SIGNED_REQUEST,  // Not supported for all win
                             // versions e.g. not for xp
                             &signed_request, CRYPT_ENCODE_ALLOC_FLAG,
                             &encode_para, &encoded_signed_request,
                             &encoded_signed_request_size);
  if (!done) {
    VLOG(ERROR) << "CryptEncodeObjectEx failed with error: " << GetLastError();
    return std::vector<GURL>();
  }

  std::unique_ptr<BYTE, base::FreeDeleter> scoped_encoded_signed_request(
      encoded_signed_request);
  std::string encoded_bin(&encoded_signed_request[0],
                          &encoded_signed_request[encoded_signed_request_size]);
  std::string base64_encoded;
  base::Base64Encode(encoded_bin, &base64_encoded);
  std::vector<GURL> result;
  for (DWORD i = 0; i < urls_from_cert.size(); ++i) {
    // This is in path but crypto API on win seems to be escaping like query.
    std::string escaped = net::EscapeQueryParamValue(base64_encoded, false);
    if (urls_from_cert[i].spec().c_str()[urls_from_cert[i].spec().length() -
                                         1] != '/')
      escaped.insert(0, "/");
    GURL result_url = GURL(urls_from_cert[0].spec() + escaped);
    result.push_back(result_url);
  }
  return result;
}

std::vector<GURL> GetCRLURLs(const net::X509Certificate::OSCertHandle& cert) {
  DWORD urls_num = 0;
  if (!CryptGetObjectUrl(
          URL_OID_CERTIFICATE_CRL_DIST_POINT,
          reinterpret_cast<LPVOID>(const_cast<PCERT_CONTEXT>(cert)),
          CRYPT_GET_URL_FROM_PROPERTY | CRYPT_GET_URL_FROM_EXTENSION |
              CRYPT_GET_URL_FROM_AUTH_ATTRIBUTE |
              CRYPT_GET_URL_FROM_UNAUTH_ATTRIBUTE,
          NULL, &urls_num, NULL, NULL, 0))
    return std::vector<GURL>();

  CRYPT_URL_ARRAY* urls =
      reinterpret_cast<CRYPT_URL_ARRAY*>(crypto::CryptAlloc(urls_num));
  std::unique_ptr<CRYPT_URL_ARRAY, base::FreeDeleter> scoped_urls(urls);
  CryptGetObjectUrl(URL_OID_CERTIFICATE_CRL_DIST_POINT,
                    reinterpret_cast<LPVOID>(const_cast<PCERT_CONTEXT>(cert)),
                    CRYPT_GET_URL_FROM_PROPERTY | CRYPT_GET_URL_FROM_EXTENSION |
                        CRYPT_GET_URL_FROM_AUTH_ATTRIBUTE |
                        CRYPT_GET_URL_FROM_UNAUTH_ATTRIBUTE,
                    urls, &urls_num, NULL, NULL, 0);

  std::vector<GURL> result;
  for (DWORD i = 0; i < urls->cUrl; ++i) {
    base::string16 url_wide_str(urls->rgwszUrl[i]);
    result.push_back(GURL(url_wide_str));
  }

  return result;
}

bool SetRevocationCheckResponse(const net::X509Certificate::OSCertHandle& cert,
                                const std::string& data,
                                bool is_ocsp_response) {
  if (data.empty())
    return false;

  if (is_ocsp_response) {
    CRYPT_DATA_BLOB ocsp_response_blob;
    ocsp_response_blob.cbData = data.size();
    ocsp_response_blob.pbData =
        reinterpret_cast<BYTE*>(const_cast<char*>(data.data()));
    // Use the same mechanism OCSP stapling uses.
    return CertSetCertificateContextProperty(
               cert, CERT_OCSP_RESPONSE_PROP_ID,
               CERT_SET_PROPERTY_IGNORE_PERSIST_ERROR_FLAG,
               &ocsp_response_blob) == TRUE;
  } else {
    // For the CRL to be use by the system it has to be placed in CA store.
    // See https://technet.microsoft.com/en-us/library/ee619754(v=ws.10).aspx.
    const void* out = nullptr;
    CERT_BLOB cert_blob;
    cert_blob.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(data.data()));
    cert_blob.cbData = data.size();
    BOOL done = CryptQueryObject(
        CERT_QUERY_OBJECT_BLOB, &cert_blob, CERT_QUERY_CONTENT_FLAG_CRL,
        CERT_QUERY_FORMAT_FLAG_BINARY, 0, 0, 0, 0, 0, 0, &out);
    const CRL_CONTEXT* crl_context = reinterpret_cast<const CRL_CONTEXT*>(out);
    if (done) {
      HCERTSTORE ca_store = CertOpenSystemStore(NULL, L"CA");
      if (ca_store)
        done = CertAddCRLContextToStore(ca_store, crl_context,
                                        CERT_STORE_ADD_NEWER, nullptr);
      if (!done && static_cast<long>(GetLastError()) == CRYPT_E_EXISTS)
        done = TRUE;

      return done == TRUE;
    }
  }
  return false;
}

bool GetTimeValidRevocationCheckResponseFromCache(const GURL& url,
                                                  bool is_ocsp,
                                                  std::string* data) {
  LPVOID* to_use = nullptr;
  CRYPT_BLOB_ARRAY* ocsp = nullptr;
  CRL_CONTEXT* crl = nullptr;
  if (is_ocsp)
    to_use = reinterpret_cast<LPVOID*>(&ocsp);
  else
    to_use = reinterpret_cast<LPVOID*>(&crl);

  if (!CryptRetrieveObjectByUrlA(
          url.spec().c_str(), is_ocsp ? CONTEXT_OID_OCSP_RESP : CONTEXT_OID_CRL,
          CRYPT_CACHE_ONLY_RETRIEVAL, 0, to_use, NULL, NULL, NULL, NULL))
    return false;

  if (is_ocsp) {
    if (ocsp->cBlob && IsTimeValidOCSPResponse(ocsp->rgBlob[0].pbData,
                                               ocsp->rgBlob[0].cbData)) {
      data->assign(reinterpret_cast<char*>(ocsp->rgBlob[0].pbData),
                   ocsp->rgBlob[0].cbData);
      return true;
    }
  } else {
    ScopedPCCRL_CONTEXT scoped_clr_context(crl);
    if (IsTimeValidCRL(crl)) {
      data->assign(reinterpret_cast<char*>(crl->pbCrlEncoded),
                   crl->cbCrlEncoded);
      return true;
    }
  }

  return false;
}

bool IsTimeValidRevocationCheckResponse(const std::string& data,
                                        bool is_ocsp_response) {
  if (!is_ocsp_response) {
    const void* out = nullptr;
    CERT_BLOB cert_blob;
    cert_blob.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(data.data()));
    cert_blob.cbData = data.size();
    if (CryptQueryObject(
            CERT_QUERY_OBJECT_BLOB, &cert_blob, CERT_QUERY_CONTENT_FLAG_CRL,
            CERT_QUERY_FORMAT_FLAG_BINARY, 0, 0, 0, 0, 0, 0, &out)) {
      const CRL_CONTEXT* crl_context =
          reinterpret_cast<const CRL_CONTEXT*>(out);
      ScopedPCCRL_CONTEXT scoped_clr_context(crl_context);
      return IsTimeValidCRL(crl_context);
    }
  } else {
    return IsTimeValidOCSPResponse(
        reinterpret_cast<BYTE*>(const_cast<char*>(data.data())), data.size());
  }
  return false;
}

}  // namespace cert_util
}  // namespace opera
