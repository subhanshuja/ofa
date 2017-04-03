// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "base/android/jni_android.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/common/page_state.h"
#include "chill/browser/navigation_history.h"
%}

namespace content {

class PageState {
 public:
  PageState();
  ~PageState();
};

%extend PageState {
  static const jbyteArray EncodePageState(content::PageState& state, bool has_post_data) {
    if (has_post_data)
        state = state.RemovePasswordData();
    const std::string& data = state.ToEncodedData();

    // Convert the string, which is actually a binary container, to a Java
    // byte[] array.
    JNIEnv* env = base::android::AttachCurrentThread();
    jbyteArray encoded_state = env->NewByteArray(data.size());
    if (encoded_state != NULL) {
      env->SetByteArrayRegion(
          encoded_state,
          0,
          data.size(),
          reinterpret_cast<const jbyte*>(data.c_str()));
    }

    return encoded_state;
  }

  static content::PageState PageStateFromEncodedData(jbyteArray encoded_state) {
    // Convert the Java byte[] array to a string.
    JNIEnv* env = base::android::AttachCurrentThread();
    size_t length = env->GetArrayLength(encoded_state);
    std::string data;
    if (length > 0) {
      jbyte *encoded_bytes = env->GetByteArrayElements(encoded_state, NULL);
      if (encoded_bytes != NULL) {
        data.assign(reinterpret_cast<const char*>(encoded_bytes), length);
        env->ReleaseByteArrayElements(encoded_state, encoded_bytes, JNI_ABORT);
      }
    }

    return content::PageState::CreateFromEncodedData(data).ReplaceLegacyOperaScheme();
  }
};

class NavigationEntry {
 public:
  void SetURL(const GURL& url);
  const GURL& GetURL() const;

  int GetUniqueID() const;

  void SetTitle(const base::string16& title);
  const base::string16& GetTitle() const;

  void SetPageID(int page_id);

  void SetVirtualURL(const GURL& url);
  const GURL& GetVirtualURL() const;

  void SetPageState(const PageState& state);
  PageState GetPageState() const;

  bool GetHasPostData() const;

  void SetIsOverridingUserAgent(bool override);

 private:
  NavigationEntry();
  ~NavigationEntry();
};
}

%template(NavigationEntryVector) std::vector<content::NavigationEntry*>;
%feature("ref")   NavigationHistory "$this->AddRef();"
%feature("unref") NavigationHistory "$this->Release();"

namespace opera {

class NavigationHistory {
 public:
  NavigationHistory();
  ~NavigationHistory();

  content::NavigationEntry* AppendNewEntry();
  int current_entry();
  void set_current_entry(int current_entry);
  const std::vector<content::NavigationEntry*>& entries() const;
};

}  // namespace opera
