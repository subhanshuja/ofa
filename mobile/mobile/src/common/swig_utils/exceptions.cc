// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Modified by Opera Software ASA
// @copied-from chromium/src/base/android/jni_android.cc

#include <string>

#include "base/logging.h"
#include "base/android/build_info.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"

using base::android::BuildInfo;

namespace {
using base::android::GetClass;
using base::android::MethodID;
using base::android::ScopedJavaLocalRef;

std::string GetJavaExceptionInfo(JNIEnv* env, jthrowable java_throwable) {
  ScopedJavaLocalRef<jclass> throwable_clazz =
      GetClass(env, "java/lang/Throwable");
  jmethodID throwable_printstacktrace =
      MethodID::Get<MethodID::TYPE_INSTANCE>(
          env, throwable_clazz.obj(), "printStackTrace",
          "(Ljava/io/PrintStream;)V");

  // Create an instance of ByteArrayOutputStream.
  ScopedJavaLocalRef<jclass> bytearray_output_stream_clazz =
      GetClass(env, "java/io/ByteArrayOutputStream");
  jmethodID bytearray_output_stream_constructor =
      MethodID::Get<MethodID::TYPE_INSTANCE>(
          env, bytearray_output_stream_clazz.obj(), "<init>", "()V");
  jmethodID bytearray_output_stream_tostring =
      MethodID::Get<MethodID::TYPE_INSTANCE>(
          env, bytearray_output_stream_clazz.obj(), "toString",
          "()Ljava/lang/String;");
  ScopedJavaLocalRef<jobject> bytearray_output_stream(env,
      env->NewObject(bytearray_output_stream_clazz.obj(),
                     bytearray_output_stream_constructor));

  // Create an instance of PrintStream.
  ScopedJavaLocalRef<jclass> printstream_clazz =
      GetClass(env, "java/io/PrintStream");
  jmethodID printstream_constructor =
      MethodID::Get<MethodID::TYPE_INSTANCE>(
          env, printstream_clazz.obj(), "<init>",
          "(Ljava/io/OutputStream;)V");
  ScopedJavaLocalRef<jobject> printstream(env,
      env->NewObject(printstream_clazz.obj(), printstream_constructor,
                     bytearray_output_stream.obj()));

  // Call Throwable.printStackTrace(PrintStream)
  env->CallVoidMethod(java_throwable, throwable_printstacktrace,
      printstream.obj());

  // Call ByteArrayOutputStream.toString()
  ScopedJavaLocalRef<jstring> exception_string(
      env, static_cast<jstring>(
          env->CallObjectMethod(bytearray_output_stream.obj(),
                                bytearray_output_stream_tostring)));

  return ConvertJavaStringToUTF8(exception_string);
}

}  // namespace

const char* GetStoredException() {
  return BuildInfo::GetInstance()->java_exception_info();
}

void HandleDirectorException(JNIEnv* env, jobject swigjobj) {
  jthrowable java_throwable = env->ExceptionOccurred();

  env->ExceptionDescribe();
  env->ExceptionClear();

  std::string info = GetJavaExceptionInfo(env, java_throwable);

  LOG(ERROR) << "Unhandled Java Exception. " << info;
  BuildInfo::GetInstance()->SetJavaExceptionInfo(info);

  // At this point we'll be holding on to a local reference to
  // swigjobj which points to the Java 'this' object. All
  // SwigDirector_.*::.* methods make sure to release this reference
  // before exiting. Since we do an early return, we must make sure
  // to do this as well.
  env->DeleteLocalRef(swigjobj);

  CHECK(false);
}
