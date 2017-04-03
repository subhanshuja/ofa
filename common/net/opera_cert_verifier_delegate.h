// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_NET_OPERA_CERT_VERIFIER_DELEGATE_H_
#define COMMON_NET_OPERA_CERT_VERIFIER_DELEGATE_H_

#include <string>
#include <vector>

#include "base/stl_util.h"
#include "base/memory/ref_counted_memory.h"
#include "net/base/net_errors.h"
#include "net/cert/cert_net_fetcher.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/cert_verify_proc.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/crl_set.h"
#include "net/cert/multi_threaded_cert_verifier.h"
#include "net/log/net_log_with_source.h"

namespace net {
class URLRequestContextGetter;
class X509Certificate;
}

namespace opera {

class OperaCertVerifierDelegate : public net::CertVerifier,
                                  public base::NonThreadSafe {
 public:
  typedef base::Callback<void(const std::string&,
                              const scoped_refptr<net::X509Certificate>&)>
      UnknownRootCallback;
  explicit OperaCertVerifierDelegate(
      std::unique_ptr<net::CertVerifier> cert_verifier);
  ~OperaCertVerifierDelegate() override;

  static void SetCertificateRevocationList(scoped_refptr<net::CRLSet> crl_set);
  static scoped_refptr<net::CRLSet> GetCertificateRevocationList();

  void SetUnknownRootCallback(UnknownRootCallback unknown_root_callback);

  // Adds the certificate identified by |public_key_hash| to the pinset for
  // |hostname_pattern|.  If there is at least one pin specified for a hostname
  // that matches |hostname_pattern|, certificate validation will only succeed
  // if at least one pin matches a certificate from the chain.
  void AddPinForHost(const std::string& hostname_pattern,
                     const std::string& public_key_hash);

  // CertVerifier implementation
  int Verify(const RequestParams& params,
             net::CRLSet* crl_set,
             net::CertVerifyResult* verify_result,
             const net::CompletionCallback& callback,
             std::unique_ptr<CertVerifier::Request>* out_req,
             const net::NetLogWithSource& net_log) override;

  void CreateFreedomRevocationFetcher(
      scoped_refptr<net::URLRequestContextGetter> context_getter);
  void DestroyFreedomRevocationFetcher();
  void ClearFreedomRevocationCache(const base::Time& begin,
                                   const base::Time& end);
  void SetFreedomRevocationFetcherExceptions(
      const std::vector<std::string>& patterns);

 private:
  struct VerifyRequestInfo {
    VerifyRequestInfo(net::CertVerifyResult* verify_result,
                      net::CompletionCallback callback,
                      OperaCertVerifierDelegate* owner,
                      std::string hostname);

    virtual ~VerifyRequestInfo();

    // Callback for when the request to |cert_verifier_| completes, so we
    // dispatch to the user's callback.
    void OnVerifyCompletion(int result);

    net::CertVerifyResult* verify_result;
    net::CompletionCallback callback;
    net::CertVerifier::Request* req;
    OperaCertVerifierDelegate* owner;
    std::string hostname;
  };

  struct RevocationCheckDownloadRequests;
  struct RevocationCheckDownloadRequest {
    RevocationCheckDownloadRequest(
        RevocationCheckDownloadRequests* owner,
        const net::X509Certificate::OSCertHandle& cert_being_checked);
    ~RevocationCheckDownloadRequest();
    // OS handle to the certificate being checked i.e. one of the certificates
    // being on |main_cert| verification path.
    net::X509Certificate::OSCertHandle cert_being_checked_os_handle;
    std::unique_ptr<net::CertNetFetcher::Request> request;
    RevocationCheckDownloadRequests* owner;
    void OnRevocationCheckDownloadResult(const std::vector<GURL>& urls,
                                         size_t index,
                                         bool ocsp,
                                         bool from_cache,
                                         net::Error err,
                                         const std::vector<uint8_t>& data);
  };

  struct RevocationCheckDownloadRequests : public CertVerifier::Request,
                                           public VerifyRequestInfo {
    RevocationCheckDownloadRequests(net::CertVerifyResult* verify_result,
                                    net::CompletionCallback callback,
                                    OperaCertVerifierDelegate* owner,
                                    const RequestParams& params,
                                    std::unique_ptr<CertVerifier::Request>* out_req,
                                    const net::NetLogWithSource& net_log);
    ~RevocationCheckDownloadRequests() override;

    // Stuff needed to be passed to VerifyInternal() after revocation check is
    // downloaded.
    std::unique_ptr<CertVerifier::Request>* out_req;
    net::NetLogWithSource net_log;
    RequestParams params;
    std::vector<RevocationCheckDownloadRequest*> requests;
  };

  // Associates a hostname pattern with a set of pins (certificate public key
  // hashes).
  struct Pinset {
    Pinset();
    Pinset(const Pinset&);
    ~Pinset();

    std::string hostname_pattern;
    net::HashValueVector acceptable_certs;
  };

  int VerifyInternal(const RequestParams& params,
                     net::CRLSet* crl_set,
                     net::CertVerifyResult* verify_result,
                     const net::CompletionCallback& callback,
                     std::unique_ptr<CertVerifier::Request>* out_req,
                     const net::NetLogWithSource& net_log,
                     bool did_revocation_check);

  int DoOperaChecks(int result,
                    const std::string& hostname,
                    net::CertVerifyResult* verify_result);

  // Checks the public keys from verify_result against
  // opera certificate revocation list. This check currently does not check
  // the serial numbers in in CRLSet
  int CheckBlacklist(net::CertVerifyResult* verify_result);

  // Checks the public keys from |verify_result| against any pins defined for
  // |hostname| by AddPinForHost().
  int CheckPublicKeyPins(const std::string& hostname,
                         const net::CertVerifyResult& verify_result);

  std::unique_ptr<RevocationCheckDownloadRequests> MaybeDownloadRevocationCheckInfo(
      const RequestParams& params,
      net::CertVerifyResult* verify_result,
      const net::CompletionCallback& callback,
      std::unique_ptr<CertVerifier::Request>* out_req,
      const net::NetLogWithSource& net_log,
      bool did_revocation);
  scoped_refptr<base::RefCountedString> GetCachedRevocationCheckInfo(
      const GURL& url,
      bool is_ocsp);
  bool IsFreedomExcludedHost(const std::string& hostname) const;

  std::unique_ptr<net::CertVerifier> cert_verifier_;

  std::vector<Pinset> pinsets_;

  // The current ongoing verify requests
  std::map<CertVerifier::Request*, VerifyRequestInfo*> requests_;
  UnknownRootCallback unknown_root_callback_;
  std::unique_ptr<net::CertNetFetcher> freedom_revocation_fetcher_;
  scoped_refptr<net::URLRequestContextGetter>
      freedom_revocation_context_getter_;
  std::vector<std::string> freedom_crl_fetching_exceptions_;
};

}  // namespace opera

#endif  // COMMON_NET_OPERA_CERT_VERIFIER_DELEGATE_H_
