// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SESSION_WINDOW_H_
#define COMMON_SYNC_SESSION_WINDOW_H_

#include <jni.h>

#include <string>

#include "base/android/scoped_java_ref.h"

namespace mobile {

class SessionWindow {
 public:
  static bool RegisterJni(JNIEnv* env);

  static base::android::ScopedJavaLocalRef<jobjectArray> GetSessionWindows(
      JNIEnv* env,
      const std::string& session_tag);
};

}  // namespace mobile

#endif  // COMMON_SYNC_SESSION_WINDOW_H_
