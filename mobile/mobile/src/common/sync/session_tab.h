// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SESSION_TAB_H_
#define COMMON_SYNC_SESSION_TAB_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"

namespace sessions {
class SessionWindow;
}

namespace mobile {

class SessionTab {
 public:
  static base::android::ScopedJavaLocalRef<jobject> GetSessionTab(
      JNIEnv* env,
      const sessions::SessionWindow* window,
      int index);

  static base::android::ScopedJavaLocalRef<jobjectArray> GetSessionTabs(
      JNIEnv* env,
      const sessions::SessionWindow* window);
};

}  // namespace mobile

#endif  // COMMON_SYNC_SESSION_TAB_H_
