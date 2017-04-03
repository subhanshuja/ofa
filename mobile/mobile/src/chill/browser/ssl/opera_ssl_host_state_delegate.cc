// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// NOLINTNEXTLINE(whitespace/line_length)
// @copied-from
// chromium/src/android_webview/browser/aw_ssl_host_state_delegate.cc
// Parts of this file is copied from
// chromium/src/chrome/browser/ssl/chrome_ssl_host_state_delegate.cc

#include "chill/browser/ssl/opera_ssl_host_state_delegate.h"

#include "base/callback.h"
#include "net/base/hash_value.h"

using content::SSLHostStateDelegate;

namespace {

net::SHA256HashValue GetChainFingerprint256(const net::X509Certificate& cert) {
  net::SHA256HashValue fingerprint =
      net::X509Certificate::CalculateChainFingerprint256(
          cert.os_cert_handle(), cert.GetIntermediateCertificates());
  return fingerprint;
}

}  // namespace

namespace opera {

OperaSSLHostStateDelegate::CertPolicy::CertPolicy() {}

OperaSSLHostStateDelegate::CertPolicy::~CertPolicy() {}

OperaSSLHostStateDelegate::OperaSSLHostStateDelegate() {}

OperaSSLHostStateDelegate::~OperaSSLHostStateDelegate() {}

void OperaSSLHostStateDelegate::AllowCert(const std::string& host,
                                          const net::X509Certificate& cert,
                                          net::CertStatus error) {
  cert_policy_for_host_[host].Allow(cert, error);
}

void OperaSSLHostStateDelegate::Clear(
    const base::Callback<bool(const std::string&)>& host_filter) {
  if (host_filter.is_null()) {
    cert_policy_for_host_.clear();
    return;
  }

  for (auto it = cert_policy_for_host_.begin();
       it != cert_policy_for_host_.end();) {
    auto next_it = std::next(it);

    if (host_filter.Run(it->first))
      cert_policy_for_host_.erase(it);

    it = next_it;
  }
}

// For an allowance, we consider a given |cert| to be a match to a saved
// allowed cert if the |error| is an exact match to or subset of the errors
// in the saved CertStatus.
bool OperaSSLHostStateDelegate::CertPolicy::Check(
    const net::X509Certificate& cert,
    net::CertStatus error) const {
  net::SHA256HashValue fingerprint = GetChainFingerprint256(cert);
  std::map<net::SHA256HashValue, net::CertStatus,
           net::SHA256HashValueLessThan>::const_iterator allowed_iter =
      allowed_.find(fingerprint);
  if ((allowed_iter != allowed_.end()) && (allowed_iter->second & error) &&
      ((allowed_iter->second & error) == error)) {
    return true;
  }
  return false;
}

void OperaSSLHostStateDelegate::CertPolicy::Allow(
    const net::X509Certificate& cert,
    net::CertStatus error) {
  // If this same cert had already been saved with a different error status,
  // this will replace it with the new error status.
  net::SHA256HashValue fingerprint = GetChainFingerprint256(cert);
  allowed_[fingerprint] = error;
}

void OperaSSLHostStateDelegate::HostRanInsecureContent(
    const std::string& host,
    int child_id,
    InsecureContentType content_type) {
  switch (content_type) {
    case MIXED_CONTENT:
      ran_mixed_content_hosts_.insert(BrokenHostEntry(host, child_id));
      return;
    case CERT_ERRORS_CONTENT:
      ran_content_with_cert_errors_hosts_.insert(
          BrokenHostEntry(host, child_id));
      return;
  }
}

bool OperaSSLHostStateDelegate::DidHostRunInsecureContent(
    const std::string& host,
    int child_id,
    InsecureContentType content_type) const {
  switch (content_type) {
    case MIXED_CONTENT:
      return !!ran_mixed_content_hosts_.count(BrokenHostEntry(host, child_id));
    case CERT_ERRORS_CONTENT:
      return !!ran_content_with_cert_errors_hosts_.count(
          BrokenHostEntry(host, child_id));
  }
  NOTREACHED();
  return false;
}

SSLHostStateDelegate::CertJudgment OperaSSLHostStateDelegate::QueryPolicy(
    const std::string& host,
    const net::X509Certificate& cert,
    net::CertStatus error,
    bool* expired_previous_decision) {
  return cert_policy_for_host_[host].Check(cert, error)
             ? SSLHostStateDelegate::ALLOWED
             : SSLHostStateDelegate::DENIED;
}

void OperaSSLHostStateDelegate::RevokeUserAllowExceptions(
    const std::string& host) {
  cert_policy_for_host_.erase(host);
}

bool OperaSSLHostStateDelegate::HasAllowException(
    const std::string& host) const {
  auto policy_iterator = cert_policy_for_host_.find(host);
  return policy_iterator != cert_policy_for_host_.end() &&
         policy_iterator->second.HasAllowException();
}

}  // namespace opera
