// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#include "chill/browser/articles/article.h"

#include "base/android/jni_string.h"
#include "jni/Article_jni.h"

namespace opera {

Article::Article(JNIEnv* env,
                 const base::android::JavaParamRef<jobject>& article)
    : article_(env, article) {}

base::android::ScopedJavaGlobalRef<jobject> Article::GetObject() {
  return article_;
}

std::string Article::GetNewsId(JNIEnv* env) {
  return base::android::ConvertJavaStringToUTF8(
      Java_Article_getNewsId(env, article_.obj()));
}

std::string Article::GetOriginalUrl(JNIEnv* env) {
  return base::android::ConvertJavaStringToUTF8(
      Java_Article_getOriginalUrl(env, article_.obj()));
}

}  // namespace opera
