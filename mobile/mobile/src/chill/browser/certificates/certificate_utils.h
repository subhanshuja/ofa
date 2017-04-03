// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from
// chromium/src/chrome/browser/ui/android/connection_info_popup_android.h
// @last-synchronized de8a62f7958001b2b223da63b9d9d872f9d3595d
//
// Modifications:
// - all unused methods removed
// - method GetCertificateChain change
// - Opera namespace added
// - Include guards names changed
// - File renamed from connection_info_popup_android.h to
//   certificate_utils.h

#ifndef CHILL_BROWSER_CERTIFICATES_CERTIFICATE_UTILS_H_
#define CHILL_BROWSER_CERTIFICATES_CERTIFICATE_UTILS_H_

#include "base/android/jni_android.h"
#include "base/strings/string16.h"

namespace opera {

jobjectArray GetCertificateChain(jobject java_web_contents);
// See opera::cert_util::GetConnectionStatusInfo
base::string16 GetConnectionStatusInfo(jobject java_web_contents);

}  // namespace opera

#endif  // CHILL_BROWSER_CERTIFICATES_CERTIFICATE_UTILS_H_
