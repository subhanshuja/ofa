// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from
// chromium/src/chrome/browser/ui/android/connection_info_popup_android.cc
// @last-synchronized de8a62f7958001b2b223da63b9d9d872f9d3595d
//
// Modifications:
// - all unused methods removed
// - method GetCertificateChain change
// - Opera namespace added
// - no-certificate check added
// - File renamed from connection_info_popup_android.cc to
//   certificate_utils.cc

#include "chill/browser/certificates/certificate_utils.h"

#include <string>
#include <vector>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/scoped_java_ref.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "common/net/certificate_util.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/ssl_status.h"
#include "content/public/browser/web_contents.h"
#include "net/cert/x509_certificate.h"

using base::android::AttachCurrentThread;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;

namespace opera {

jobjectArray GetCertificateChain(jobject j_web_contents) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(
          ScopedJavaLocalRef<jobject>(AttachCurrentThread(), j_web_contents));
  if (!web_contents)
    return jobjectArray();

  scoped_refptr<net::X509Certificate> cert =
      web_contents->GetController().GetVisibleEntry()->GetSSL().certificate;
  if (!cert) {
    // No certificate available, return empty bytes instead of crashing.
    return jobjectArray();
  }

  std::vector<std::string> cert_chain;
  net::X509Certificate::OSCertHandles cert_handles =
      cert->GetIntermediateCertificates();
  // Make sure the peer's own cert is the first in the chain, if it's not
  // already there.
  if (cert_handles.empty() || cert_handles[0] != cert->os_cert_handle())
    cert_handles.insert(cert_handles.begin(), cert->os_cert_handle());

  cert_chain.reserve(cert_handles.size());
  for (net::X509Certificate::OSCertHandles::const_iterator it =
           cert_handles.begin();
       it != cert_handles.end();
       ++it) {
    std::string cert_bytes;
    net::X509Certificate::GetDEREncoded(*it, &cert_bytes);
    cert_chain.push_back(cert_bytes);
  }

  JNIEnv* env = AttachCurrentThread();
  return base::android::ToJavaArrayOfByteArray(env, cert_chain).Release();
}

base::string16 GetConnectionStatusInfo(jobject j_web_contents) {
  content::WebContents* web_contents =
       content::WebContents::FromJavaWebContents(
          ScopedJavaLocalRef<jobject>(AttachCurrentThread(), j_web_contents));
  if (!web_contents)
    return base::string16();

  content::SSLStatus ssl =
       web_contents->GetController().GetVisibleEntry()->GetSSL();
  if (!ssl.certificate) {
    return base::string16();
  }
  return base::UTF8ToUTF16(
       opera::cert_util::GetConnectionStatusInfo(ssl));
}

}  // namespace opera
