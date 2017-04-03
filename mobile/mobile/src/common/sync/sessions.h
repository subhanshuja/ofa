// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SESSIONS_H_
#define COMMON_SYNC_SESSIONS_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"

namespace sync_sessions {
class OpenTabsUIDelegate;
}  // namespace sync_sessions

namespace mobile {

class Sessions {
 public:
  static bool RegisterJni(JNIEnv* env);

  static base::android::ScopedJavaLocalRef<jobjectArray> GetSessions(
      JNIEnv* env);

  static sync_sessions::OpenTabsUIDelegate* GetOpenTabsUIDelegate();
};

}  // namespace mobile

#endif  // COMMON_SYNC_SESSIONS_H_
