// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/cobranding/obml_request.h"

#include <queue>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/native_library.h"

#include "content/public/browser/browser_thread.h"

#include "jni/LibraryManager_jni.h"

namespace {

const char* kRequestFunctionName = "MakeBlindOBMLRequest";

// OBML requests can only be made from libom.so, which is why this cheat exists.
// It loads the libom.so library (which is shamelessly assumed to already be
// loaded) and uses dlsym() to find the appropriate function therein.
class OBMLRequester {
 public:
  OBMLRequester();

  void MakeBlindOBMLRequest(const char* url);

 private:
  typedef void(request_fn_t)(const char*);

  void SetFunctionOnFileThread(const base::FilePath& libom_path);
  void DoPendingRequestsOnIOThread();

  std::queue<std::string> pending_requests_;
  base::Callback<void(const char*)> request_function_;
};

OBMLRequester::OBMLRequester() {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> path =
    Java_LibraryManager_findMiniLibrary(env);
  base::FilePath libom_path(ConvertJavaStringToUTF8(path));

  content::BrowserThread::PostTaskAndReply(
      content::BrowserThread::FILE, FROM_HERE,
      base::Bind(&OBMLRequester::SetFunctionOnFileThread,
                 base::Unretained(this), libom_path),
      base::Bind(&OBMLRequester::DoPendingRequestsOnIOThread,
                 base::Unretained(this)));
}

void OBMLRequester::MakeBlindOBMLRequest(const char* url) {
  if (request_function_.is_null()) {
    pending_requests_.push(url);
  } else {
    request_function_.Run(url);
  }
}

void OBMLRequester::SetFunctionOnFileThread(const base::FilePath& libom_path) {
  base::NativeLibraryLoadError load_error;
  base::NativeLibrary libom = base::LoadNativeLibrary(libom_path, &load_error);
  if (!libom) {
    LOG(FATAL) << "Couldn't load " << libom_path.value() << ": "
               << load_error.ToString();
  }

  void* fn_ptr =
      base::GetFunctionPointerFromNativeLibrary(libom, kRequestFunctionName);
  DCHECK(fn_ptr);

  request_function_ = base::Bind(reinterpret_cast<request_fn_t*>(fn_ptr));
}

void OBMLRequester::DoPendingRequestsOnIOThread() {
  while (!pending_requests_.empty()) {
    request_function_.Run(pending_requests_.front().c_str());
    pending_requests_.pop();
  }
}

}  // namespace

namespace opera {

void MakeBlindOBMLRequest(const std::string& url) {
  static OBMLRequester* requester = new OBMLRequester();
  requester->MakeBlindOBMLRequest(url.c_str());
}

}  // namespace opera
