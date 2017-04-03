// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#ifndef CHILL_BROWSER_ARTICLES_ARTICLE_H_
#define CHILL_BROWSER_ARTICLES_ARTICLE_H_

#include <jni.h>

#include <string>

#include "base/android/scoped_java_ref.h"

namespace opera {

class Article {
 public:
  Article(JNIEnv* env, const base::android::JavaParamRef<jobject>& article);

  base::android::ScopedJavaGlobalRef<jobject> GetObject();
  std::string GetNewsId(JNIEnv* env);
  std::string GetOriginalUrl(JNIEnv* env);

 private:
  const base::android::ScopedJavaGlobalRef<jobject> article_;
};

}  // namespace opera

#endif  // CHILL_BROWSER_ARTICLES_ARTICLE_H_
