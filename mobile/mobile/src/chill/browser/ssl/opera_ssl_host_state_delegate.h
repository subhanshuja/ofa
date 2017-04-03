// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// NOLINTNEXTLINE(whitespace/line_length)
// @copied-from
// chromium/src/android_webview/browser/aw_ssl_host_state_delegate.h
// Parts of this file is copied from
// chromium/src/chrome/browser/ssl/chrome_ssl_host_state_delegate.h

#ifndef CHILL_BROWSER_SSL_OPERA_SSL_HOST_STATE_DELEGATE_H_
#define CHILL_BROWSER_SSL_OPERA_SSL_HOST_STATE_DELEGATE_H_

#include <set>
#include <map>
#include <string>
#include <utility>

#include "content/public/browser/ssl_host_state_delegate.h"
#include "net/base/hash_value.h"
#include "net/cert/cert_status_flags.h"
#include "net/cert/x509_certificate.h"

namespace opera {

class OperaSSLHostStateDelegate : public content::SSLHostStateDelegate {
 public:
  OperaSSLHostStateDelegate();
  ~OperaSSLHostStateDelegate() override;

  // Records that |cert| is permitted to be used for |host| in the future, for
  // a specified |error| type.
  void AllowCert(const std::string& host,
                 const net::X509Certificate& cert,
                 net::CertStatus error) override;

  void Clear(
      const base::Callback<bool(const std::string&)>& host_filter) override;

  // Queries whether |cert| is allowed or denied for |host| and |error|.
  content::SSLHostStateDelegate::CertJudgment QueryPolicy(
      const std::string& host,
      const net::X509Certificate& cert,
      net::CertStatus error,
      bool* expired_previous_decision) override;

  // Records that a host has run insecure content of the given |content_type|.
  void HostRanInsecureContent(const std::string& host,
                              int child_id,
                              InsecureContentType content_type);

  // Returns whether the specified host ran insecure content.
  bool DidHostRunInsecureContent(
      const std::string& host,
      int child_id,
      InsecureContentType content_type) const override;

  // Revokes all SSL certificate error allow exceptions made by the user for
  // |host|.
  void RevokeUserAllowExceptions(const std::string& host) override;

  // Returns whether the user has allowed a certificate error exception for
  // |host|. This does not mean that *all* certificate errors are allowed, just
  // that there exists an exception. To see if a particular certificate and
  // error combination exception is allowed, use QueryPolicy().
  bool HasAllowException(const std::string& host) const override;

 private:
  // This class maintains the policy for storing actions on certificate errors.
  class CertPolicy {
   public:
    CertPolicy();
    ~CertPolicy();
    // Returns true if the user has decided to proceed through the ssl error
    // before. For a certificate to be allowed, it must not have any
    // *additional* errors from when it was allowed.
    bool Check(const net::X509Certificate& cert, net::CertStatus error) const;

    // Causes the policy to allow this certificate for a given |error|. And
    // remember the user's choice.
    void Allow(const net::X509Certificate& cert, net::CertStatus error);

    // Returns true if and only if there exists a user allow exception for some
    // certificate.
    bool HasAllowException() const { return allowed_.size() > 0; }

   private:
    // The set of fingerprints of allowed certificates.
    std::map<net::SHA256HashValue,
             net::CertStatus,
             net::SHA256HashValueLessThan>
        allowed_;
  };

  // Certificate policies for each host.
  std::map<std::string, CertPolicy> cert_policy_for_host_;

  // A BrokenHostEntry is a pair of (host, process_id) that indicates the host
  // contains insecure content in that renderer process.
  typedef std::pair<std::string, int> BrokenHostEntry;

  // Hosts which have been contaminated with insecure mixed content in the
  // specified process.  Note that insecure content can travel between
  // same-origin frames in one processs but cannot jump between processes.
  std::set<BrokenHostEntry> ran_mixed_content_hosts_;

  // Hosts which have been contaminated with content with certificate errors in
  // the specific process.
  std::set<BrokenHostEntry> ran_content_with_cert_errors_hosts_;

  DISALLOW_COPY_AND_ASSIGN(OperaSSLHostStateDelegate);
};

}  // namespace opera

#endif  // CHILL_BROWSER_SSL_OPERA_SSL_HOST_STATE_DELEGATE_H_
