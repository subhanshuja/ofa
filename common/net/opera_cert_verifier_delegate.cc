// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/net/opera_cert_verifier_delegate.h"

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/strings/pattern.h"
#include "base/synchronization/lock.h"
#include "crypto/sha2.h"
#include "net/base/net_errors.h"
#include "net/base/load_flags.h"
#include "net/cert/crl_set.h"
#include "net/cert/x509_cert_types.h"
#include "net/cert_net/cert_net_fetcher_impl.h"
#include "net/disk_cache/disk_cache.h"
#include "net/http/http_cache.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

#include "common/net/certificate_util.h"

using net::HashValueVector;
using net::HashValueTag;
using net::CRLSet;

namespace opera {

namespace {

bool HashesIntersect(const net::HashValueVector& x,
                     const net::HashValueVector& y) {
  return std::find_first_of(
             x.begin(), x.end(), y.begin(), y.end(),
             [](const net::HashValue& x_hash, const net::HashValue& y_hash) {
               return x_hash == y_hash;
             }) != x.end();
}

struct CacheGetHelper : public base::RefCounted<CacheGetHelper> {
  CacheGetHelper() = default;
  disk_cache::Backend* cache_backend;

 private:
  friend class base::RefCounted<CacheGetHelper>;
  ~CacheGetHelper() = default;
};

void FreedomRevocationCacheCleared(int result) {}

void ClearFreedomRevocationCacheHelper(scoped_refptr<CacheGetHelper> helper,
                                       const base::Time& begin,
                                       const base::Time& end,
                                       int result) {
  if (result != net::OK)
    return;

  if (begin.is_null() && end.is_max()) {
    helper->cache_backend->DoomAllEntries(
        base::Bind(&FreedomRevocationCacheCleared));
  } else {
    helper->cache_backend->DoomEntriesBetween(
        begin, end, base::Bind(FreedomRevocationCacheCleared));
  }
}

typedef std::unique_ptr<net::CertNetFetcher::Request> (
    net::CertNetFetcher::*Fetch)(
    const GURL& url,
    int timeout_milliseconds,
    int max_response_bytes,
    int load_flags,
    const net::CertNetFetcher::FetchCallback& callback);

// GlobalCRLSet class and g_crl_set variable were copied from
// net/base/ssl_config_service.cc.
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
//
// GlobalCRLSet holds a reference to the global CRLSet. It simply wraps a lock
// around a scoped_refptr so that getting a reference doesn't race with
// updating the CRLSet.
class GlobalCRLSet {
 public:
  void Set(const scoped_refptr<CRLSet>& new_crl_set) {
    base::AutoLock locked(lock_);
    crl_set_ = new_crl_set;
  }

  scoped_refptr<CRLSet> Get() const {
    base::AutoLock locked(lock_);
    return crl_set_;
  }

 private:
  scoped_refptr<CRLSet> crl_set_;
  mutable base::Lock lock_;
};

base::LazyInstance<GlobalCRLSet>::Leaky g_crl_set = LAZY_INSTANCE_INITIALIZER;

}  // namespace

OperaCertVerifierDelegate::Pinset::Pinset() = default;

OperaCertVerifierDelegate::Pinset::Pinset(
    const OperaCertVerifierDelegate::Pinset&) = default;

OperaCertVerifierDelegate::Pinset::~Pinset() = default;

// static
void OperaCertVerifierDelegate::SetCertificateRevocationList(
    scoped_refptr<CRLSet> crl_set) {
  g_crl_set.Get().Set(crl_set);
}

// static
scoped_refptr<CRLSet>
OperaCertVerifierDelegate::GetCertificateRevocationList() {
  return g_crl_set.Get().Get();
}

OperaCertVerifierDelegate::OperaCertVerifierDelegate(
    std::unique_ptr<CertVerifier> cert_verifier)
    : cert_verifier_(std::move(cert_verifier)),
      unknown_root_callback_(UnknownRootCallback()) {}

void OperaCertVerifierDelegate::SetUnknownRootCallback(
    UnknownRootCallback unknown_root_callback) {
  unknown_root_callback_ = unknown_root_callback;
}

void OperaCertVerifierDelegate::AddPinForHost(
    const std::string& hostname_pattern,
    const std::string& public_key_hash) {
  DVLOG(1) << __FUNCTION__ << "(" << hostname_pattern << ")";
  net::HashValue hash(net::HASH_VALUE_SHA256);
  CHECK_EQ(hash.size(), public_key_hash.size());
  std::copy(public_key_hash.data(),
            public_key_hash.data() + public_key_hash.size(), hash.data());

  net::HashValueVector* acceptable_certs = nullptr;
  const auto it =
      std::find_if(pinsets_.begin(), pinsets_.end(),
                   [&hostname_pattern](const Pinset& pinset) {
                     return pinset.hostname_pattern == hostname_pattern;
                   });
  if (it == pinsets_.end()) {
    Pinset pinset;
    pinset.hostname_pattern = hostname_pattern;
    pinsets_.emplace_back(std::move(pinset));
    acceptable_certs = &pinsets_.back().acceptable_certs;
  } else {
    acceptable_certs = &it->acceptable_certs;
  }

  acceptable_certs->emplace_back(std::move(hash));
}

OperaCertVerifierDelegate::~OperaCertVerifierDelegate() {
  base::STLDeleteValues(&requests_);
}

int OperaCertVerifierDelegate::Verify(const RequestParams& params,
                                      net::CRLSet* crl_set,
                                      net::CertVerifyResult* verify_result,
                                      const net::CompletionCallback& callback,
                                      std::unique_ptr<CertVerifier::Request>* out_req,
                                      const net::NetLogWithSource& net_log) {
  return VerifyInternal(params, crl_set, verify_result, callback, out_req,
                        net_log, false);
}

int OperaCertVerifierDelegate::VerifyInternal(
    const RequestParams& params,
    net::CRLSet* crl_set,
    net::CertVerifyResult* verify_result,
    const net::CompletionCallback& callback,
    std::unique_ptr<CertVerifier::Request>* out_req,
    const net::NetLogWithSource& net_log,
    bool did_revocation) {
  DCHECK(CalledOnValidThread());

  std::unique_ptr<RevocationCheckDownloadRequests> requests =
      MaybeDownloadRevocationCheckInfo(params, verify_result, callback, out_req,
                                       net_log, did_revocation);
  if (!requests->requests.empty()) {
    out_req->reset(requests.release());
    return net::ERR_IO_PENDING;
  }

  std::unique_ptr<VerifyRequestInfo> request_info(
      new VerifyRequestInfo(verify_result, callback, this, params.hostname()));

  int verification_result = cert_verifier_->Verify(
      params,
      crl_set,
      verify_result,
      base::Bind(
          &OperaCertVerifierDelegate::VerifyRequestInfo::OnVerifyCompletion,
          base::Unretained(request_info.get())),
      out_req, net_log);

  if (verification_result == net::ERR_IO_PENDING) {
    VerifyRequestInfo *temp_request_info = request_info.release();
    temp_request_info->req = &**out_req;
    requests_.insert(std::make_pair(&**out_req, temp_request_info));
  } else {
    verification_result = DoOperaChecks(verification_result, params.hostname(),
                                        request_info->verify_result);
  }

  return verification_result;
}

OperaCertVerifierDelegate::VerifyRequestInfo::VerifyRequestInfo(
    net::CertVerifyResult* verify_result,
    net::CompletionCallback callback,
    OperaCertVerifierDelegate* owner,
    std::string hostname)
    : verify_result(verify_result),
      callback(callback),
      req(NULL),
      owner(owner),
      hostname(hostname) {
}

void OperaCertVerifierDelegate::CreateFreedomRevocationFetcher(
    scoped_refptr<net::URLRequestContextGetter> context_getter) {
  freedom_revocation_context_getter_ = context_getter;
  freedom_revocation_fetcher_.reset(
      new net::CertNetFetcherImpl(context_getter->GetURLRequestContext()));
}

void OperaCertVerifierDelegate::DestroyFreedomRevocationFetcher() {
  freedom_revocation_context_getter_ = nullptr;
  freedom_revocation_fetcher_.reset(nullptr);
}

void OperaCertVerifierDelegate::ClearFreedomRevocationCache(
    const base::Time& begin,
    const base::Time& end) {
  if (!freedom_revocation_context_getter_)
    return;

  net::URLRequestContext* context =
      freedom_revocation_context_getter_->GetURLRequestContext();
  net::HttpCache* http_cache = context->http_transaction_factory()->GetCache();
  scoped_refptr<CacheGetHelper> helper = new CacheGetHelper;
  int rv = http_cache
               ? http_cache->GetBackend(
                     &helper->cache_backend,
                     base::Bind(&ClearFreedomRevocationCacheHelper, helper,
                                begin, end))
               : net::ERR_CACHE_OPEN_FAILURE;
  if (rv != net::ERR_IO_PENDING)
    ClearFreedomRevocationCacheHelper(helper, begin, end, rv);
}

void OperaCertVerifierDelegate::SetFreedomRevocationFetcherExceptions(
    const std::vector<std::string>& patterns) {
  freedom_crl_fetching_exceptions_ = patterns;
}

OperaCertVerifierDelegate::RevocationCheckDownloadRequest::
    RevocationCheckDownloadRequest(
        RevocationCheckDownloadRequests* owner,
        const net::X509Certificate::OSCertHandle& cert_to_check)
    : cert_being_checked_os_handle(cert_to_check), owner(owner) {}

OperaCertVerifierDelegate::RevocationCheckDownloadRequest::
    ~RevocationCheckDownloadRequest() {
  net::X509Certificate::FreeOSCertHandle(cert_being_checked_os_handle);
}

void OperaCertVerifierDelegate::RevocationCheckDownloadRequest::
    OnRevocationCheckDownloadResult(const std::vector<GURL>& urls,
                                    size_t index,
                                    bool ocsp,
                                    bool from_cache,
                                    net::Error err,
                                    const std::vector<uint8_t>& data) {
  std::string response(data.begin(), data.end());
  const std::vector<GURL>* urls_to_use = &urls;
  std::vector<GURL> crl_urls;
  if (from_cache &&
      (err != net::OK ||
       !cert_util::IsTimeValidRevocationCheckResponse(response, ocsp))) {
    // This will cause the request is repeated but bypassing cache.
    err = net::ERR_CACHE_MISS;
  }

  // In case of an error try the next URL if possible.
  if (err != net::OK) {
    if (err != net::ERR_CACHE_MISS && ++index >= urls.size()) {
      // No more urls...
      urls_to_use = nullptr;
      // ... but if it's OCSP try falling back to CRL
      if (ocsp) {
        ocsp = false;
        index = 0;
        crl_urls = cert_util::GetCRLURLs(cert_being_checked_os_handle);
        if (!crl_urls.empty())
          urls_to_use = &crl_urls;
      }
    }

    if (urls_to_use) {
      scoped_refptr<base::RefCountedString> os_cached =
          owner->owner->GetCachedRevocationCheckInfo(urls_to_use->at(index),
                                                     ocsp);

      Fetch fetch_function = ocsp ? &net::CertNetFetcher::FetchOcsp
                                  : &net::CertNetFetcher::FetchCrl;
      if (!os_cached) {
        // If there was an error and it's not cache related try loading the new
        // url from cache first too.
        bool retry_from_cache = err != net::ERR_CACHE_MISS;
        this->request =
            (owner->owner->freedom_revocation_fetcher_.get()->*fetch_function)(
                urls_to_use->at(index), net::CertNetFetcher::DEFAULT,
                net::CertNetFetcher::DEFAULT,
                retry_from_cache ? net::LOAD_PREFERRING_CACHE
                                 : net::LOAD_BYPASS_CACHE,
                base::Bind(
                    &OperaCertVerifierDelegate::RevocationCheckDownloadRequest::
                        OnRevocationCheckDownloadResult,
                    base::Unretained(this), *urls_to_use, index, ocsp,
                    retry_from_cache));
        return;
      } else {
        err = net::OK;
        response = os_cached->data();
      }
    }
  }

  if (err == net::OK) {
    bool done = cert_util::SetRevocationCheckResponse(
        cert_being_checked_os_handle, response, ocsp);
    if (!done)
      DLOG(ERROR) << "Couldn't set " << urls_to_use->at(index).spec()
                  << "'s data for the certificate";
  } else {
    DLOG(ERROR) << "Error when downloading revocation check info: " << err;
  }
  owner->requests.erase(
      std::find(owner->requests.begin(), owner->requests.end(), this));
  std::unique_ptr<OperaCertVerifierDelegate::RevocationCheckDownloadRequest>
  auto_this(
      static_cast<OperaCertVerifierDelegate::RevocationCheckDownloadRequest*>(
          this));
  if (owner->requests.empty()) {
    // owner is also stored under owner->out_req. Take it over because
    // VerifyInternal() will delete it (it resets the passed in pointer).
    // The owner must be kept alive until the call ends because the stuff
    // from it (e.g. owner->verify_result, owner->net_log etc) is passed
    // to that call.
    std::unique_ptr<CertVerifier::Request> auto_requests(owner->out_req->release());
    RequestParams params(owner->params.certificate(), owner->params.hostname(),
        owner->params.flags() & ~CertVerifier::VERIFY_REV_CHECKING_ENABLED,
        owner->params.ocsp_response(), owner->params.additional_trust_anchors());
    int result = owner->owner->VerifyInternal(
        owner->params, nullptr, owner->verify_result, owner->callback,
        owner->out_req, owner->net_log, true);
    if (result != net::ERR_IO_PENDING)
      owner->callback.Run(result);
  }
}

OperaCertVerifierDelegate::RevocationCheckDownloadRequests::
    RevocationCheckDownloadRequests(net::CertVerifyResult* verify_result,
                                    net::CompletionCallback callback,
                                    OperaCertVerifierDelegate* owner,
                                    const RequestParams& params,
                                    std::unique_ptr<CertVerifier::Request>* out_req,
                                    const net::NetLogWithSource& net_log)
    : VerifyRequestInfo(verify_result, callback, owner, params.hostname()),
      out_req(out_req),
      net_log(net_log),
      params(params) {}

OperaCertVerifierDelegate::RevocationCheckDownloadRequests::
    ~RevocationCheckDownloadRequests() {
  base::STLDeleteElements(&requests);
}

std::unique_ptr<OperaCertVerifierDelegate::RevocationCheckDownloadRequests>
OperaCertVerifierDelegate::MaybeDownloadRevocationCheckInfo(
    const RequestParams& params,
    net::CertVerifyResult* verify_result,
    const net::CompletionCallback& callback,
    std::unique_ptr<CertVerifier::Request>* out_req,
    const net::NetLogWithSource& net_log,
    bool did_revocation) {
  std::unique_ptr<RevocationCheckDownloadRequests> requests(
      new RevocationCheckDownloadRequests(verify_result, callback, this,
                                          params, out_req, net_log));
  if (freedom_revocation_fetcher_ && !did_revocation &&
      !IsFreedomExcludedHost(params.hostname())) {
    // Caller of the GetCertificateRevocationCheckChains() is reponsible for
    // freeing the returned handles. It'll happen here because the handle is
    // passed to RevocationCheckDownloadRequest which frees it in its dtor.
    cert_util::CertificateRevocationCheckChains chains =
        cert_util::GetCertificateRevocationCheckChains(
            *params.certificate().get(),
            (params.flags() & net::CertVerifier::VERIFY_EV_CERT) != 0);
    std::vector<GURL> requested_urls;
    for (auto chain : chains) {
      // Iterate from certificate just before root down.
      // Root is at size() - 1 index.
      for (int i = chain.size() - 2; i >= 0; --i) {
        std::unique_ptr<RevocationCheckDownloadRequest> download_req(
            new RevocationCheckDownloadRequest(requests.get(), chain[i]));
        Fetch fetch_function = &net::CertNetFetcher::FetchOcsp;
        std::vector<GURL> revocation_urls =
            /* Issuer of a certificate is just one level up in the chain. */
            cert_util::GetOCSPURLs(chain[i], chain[i + 1]);
        bool has_ocsp = !revocation_urls.empty();
        if (!has_ocsp) {
          revocation_urls = cert_util::GetCRLURLs(chain[i]);
          fetch_function = &net::CertNetFetcher::FetchCrl;
          if (revocation_urls.empty())
            continue;
        }

        scoped_refptr<base::RefCountedString> os_cached =
            GetCachedRevocationCheckInfo(revocation_urls[0], has_ocsp);
        if (!os_cached) {
          // Don't duplicate already requested URLs.
          if (std::find(requested_urls.begin(), requested_urls.end(),
                        revocation_urls[0]) != requested_urls.end())
            continue;

          download_req
              ->request = (freedom_revocation_fetcher_.get()->*fetch_function)(
              revocation_urls[0], net::CertNetFetcher::DEFAULT,
              net::CertNetFetcher::DEFAULT, net::LOAD_PREFERRING_CACHE,
              base::Bind(
                  &OperaCertVerifierDelegate::RevocationCheckDownloadRequest::
                      OnRevocationCheckDownloadResult,
                  base::Unretained(download_req.get()), revocation_urls, 0,
                  has_ocsp, true));
          requested_urls.push_back(revocation_urls[0]);
          requests->requests.push_back(download_req.release());
        } else {
          // Set the data straight away:
          bool done = cert_util::SetRevocationCheckResponse(
              chain[i], os_cached->data(), true);
          if (!done)
            DLOG(ERROR) << "Couldn't set " << revocation_urls[0].spec()
                        << "'s data for the certificate";
        }
      }
    }
  }

  return requests;
}

scoped_refptr<base::RefCountedString>
OperaCertVerifierDelegate::GetCachedRevocationCheckInfo(const GURL& url,
                                                        bool is_ocsp) {
  // Try OS:
  std::string data;
  if (cert_util::GetTimeValidRevocationCheckResponseFromCache(url, is_ocsp,
                                                              &data)) {
    scoped_refptr<base::RefCountedString> result =
        base::RefCountedString::TakeString(&data);
    return result;
  }

  return scoped_refptr<base::RefCountedString>();
}

bool OperaCertVerifierDelegate::IsFreedomExcludedHost(
    const std::string& hostname) const {
  for (const auto& pattern : freedom_crl_fetching_exceptions_) {
    if (base::MatchPattern(hostname, pattern))
      return true;
  }
  return false;
}

OperaCertVerifierDelegate::VerifyRequestInfo::~VerifyRequestInfo() {
  if (req)
    owner->requests_.erase(req);
}

void OperaCertVerifierDelegate::VerifyRequestInfo::OnVerifyCompletion(
    int result) {
  DCHECK(owner->CalledOnValidThread());

  result = owner->DoOperaChecks(result, hostname, verify_result);

  if (!net::IsCertStatusError(verify_result->cert_status) &&
      !verify_result->is_issued_by_known_root &&
      !owner->unknown_root_callback_.is_null()) {
    owner->unknown_root_callback_.Run(hostname,
                                      verify_result->verified_cert);
  }

  // Clear the outstanding request information.
  net::CompletionCallback temp_callback = callback;
  verify_result = nullptr;
  callback.Reset();

  // Call the user's original callback.
  temp_callback.Run(result);

  // This call is guaranteed to be the last thing that happens to this object.
  // Thus deleting this is ok. We could avoid this by posting a message to
  // OperaCertVerifierDelegate and let OperaCertVerifierDelegate delete
  // VerifyRequestInfo objects, but that would complicate the code.
  delete this;
}

int OperaCertVerifierDelegate::DoOperaChecks(
    int result,
    const std::string& hostname,
    net::CertVerifyResult* verify_result) {
  // |result| might contain an error that the user can click through.
  // Revocation and pinning errors are more severe.
  int local_result = CheckBlacklist(verify_result);
  if (local_result == net::OK)
    local_result = CheckPublicKeyPins(hostname, *verify_result);

  if (local_result != net::OK)
    result = local_result;

  return result;
}

int OperaCertVerifierDelegate::CheckBlacklist(
    net::CertVerifyResult* verify_result) {
  scoped_refptr<CRLSet> opera_certificate_revocation_list =
      OperaCertVerifierDelegate::GetCertificateRevocationList();
  if (opera_certificate_revocation_list.get()) {
    for (HashValueVector::const_iterator j = verify_result->public_key_hashes
        .begin(); j != verify_result->public_key_hashes.end(); ++j) {
      if (j->tag == net::HASH_VALUE_SHA256) {
        const base::StringPiece spki_hash = base::StringPiece(
            reinterpret_cast<const char*>(j->data()), crypto::kSHA256Length);

        if (opera_certificate_revocation_list->CheckSPKI(spki_hash)
            == net::CRLSet::REVOKED) {
          verify_result->cert_status |= net::CERT_STATUS_REVOKED;
          return net::ERR_CERT_REVOKED;
        }
      }
    }
  }

  return net::OK;
}

int OperaCertVerifierDelegate::CheckPublicKeyPins(
    const std::string& hostname,
    const net::CertVerifyResult& verify_result) {
  // TODO(wdzierzanowski): Might need to make this more efficient at some
  // point.  Compare with chromium/src/net/http/transport_security_state.cc.
  const auto it = std::find_if(
      pinsets_.begin(), pinsets_.end(), [&hostname](const Pinset& pinset) {
        return base::MatchPattern(hostname, pinset.hostname_pattern);
      });
  if (it == pinsets_.end()) {
    DVLOG(5) << "No public key pins for " << hostname;
    return net::OK;
  }

  if (HashesIntersect(verify_result.public_key_hashes, it->acceptable_certs)) {
    DVLOG(3) << "Public key pins verified for " << hostname;
    return net::OK;
  }

  DVLOG(1) << "Public key pins not matched for " << hostname;
  return net::ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN;
}

}  // namespace opera
