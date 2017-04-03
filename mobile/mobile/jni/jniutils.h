/* -*- Mode: c++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
 *
 * Copyright (C) 2014 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#ifndef JNIUTILS_H
#define JNIUTILS_H

#include <jni.h>

template<typename java_type>
class scoped_localref {
 public:
  scoped_localref()
      : env_(0), object_(0) {
  }
  explicit scoped_localref(JNIEnv* env, java_type object)
      : env_(env), object_(object) {
  }
  ~scoped_localref() {
    reset();
  }
  scoped_localref(const scoped_localref& ref)
      : env_(ref.env_) {
    object_ = ref.object_ ? (java_type) env_->NewLocalRef(ref.object_) : 0;
  }
  scoped_localref& operator=(const scoped_localref& ref) {
    reset();
    env_ = ref.env_;
    object_ = ref.object_ ? (java_type) env_->NewLocalRef(ref.object_) : 0;
    return *this;
  }
  java_type get() const {
    return object_;
  }
  java_type operator *() const {
    return get();
  }
  operator bool() const {
    return get() != 0;
  }
  java_type createGlobalRef() {
    return object_ ? env_->NewGlobalRef(object_) : 0;
  }
  void reset() {
    if (object_)
      env_->DeleteLocalRef(object_);
    object_ = 0;
  }
  java_type pass() {
    java_type ret = object_;
    object_ = 0;
    return ret;
  }
  JNIEnv* env() const {
    return env_;
  }
 private:
  JNIEnv* env_;
  java_type object_;
};

class jnistring {
 public:
  /* valid UTF-8 string, zero-terminated */
  const char *string() const {
    return string_;
  }

  operator bool() const {
    return string_ != 0;
  }

  ~jnistring() {
    reset();
  }

  jnistring()
      : string_(0) {
  }

  jnistring(scoped_localref<jstring> jstring, char *string)
      : jstring_(jstring), string_(string) {
  }

  jnistring(char *string)
      : string_(string) {
  }

  jnistring(const jnistring& str);
  jnistring& operator=(const jnistring& str);
  void reset();
  char *pass();

 private:
  scoped_localref<jstring> jstring_;
  char *string_;
};

/**
 * Convert a Java string object into an array of bytes in UTF-8 encoding.
 * Makes sure that any surrogate pairs in the Java string are correctly encoded
 * in UTF-8.
 * @param env JNI environment, may not be NULL
 * @param str Java string object, may be NULL
 * @param safe if true str is assumed to not contain any surrogate pairs so
 *             some time can be saved. Only use if you are absolutely sure.
 * @param takeOwnership if true GetStringUTF8Chars takes ownership of str
 * @return struct containing a valid UTF-8 string
 */
jnistring GetStringUTF8Chars(JNIEnv *env, jstring str,
                             bool safe = false, bool takeOwnership = false);

/**
 * Acts just like GetStringUTF8Chars but also releases str.
 */
jnistring GetAndReleaseStringUTF8Chars(JNIEnv *env, jstring str,
                                       bool safe = false);
jnistring GetAndReleaseStringUTF8Chars(scoped_localref<jstring>& str,
                                       bool safe = false);

/**
 * Convert a UTF-8 encoded string of bytes to a Java string object.
 * Makes sure that any unicode points >= 0x10000 are encoded using surrogate
 * pairs in the Java string.
 * @param env JNI environment, may not be NULL
 * @param str UTF-8 encoded string of bytes, may not be NULL
 * @param safe if true str is assumed to not contain any unicode
 *             points >= 0x10000 and some time can be saved.
 *             Only use if you are absolutely sure.
 * @return Java string object
 */
scoped_localref<jstring> NewStringUTF8(
    JNIEnv *env, const char *str, bool safe = false);

#define GetStringUTFChars DONT_USE_THIS
#define ReleaseStringUTFChars DONT_USE_THIS
#define NewStringUTF DONT_USE_THIS

#endif /* JNIUTILS_H */
