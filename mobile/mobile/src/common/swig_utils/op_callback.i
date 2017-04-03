// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "base/android/scoped_java_ref.h"
#include "base/bind.h"
#include "base/callback.h"

#include "common/swig_utils/op_arguments.h"
#include "common/swig_utils/op_runnable.h"
%}

%include <typemaps.i>

typedef base::Callback<void(const OpArguments&)> OpRunnable;

class OpArguments {
 public:
  OpArguments() {}
  virtual ~OpArguments() {}
};

namespace base {
template <class T>
class Callback;
}

%naturalvar OpArguments;

%typemap(jni) const OpArguments& "jobject";
%typemap(jtype) const OpArguments& "OpArguments";
%typemap(jstype) const OpArguments& "OpArguments";
%typemap(javain) const OpArguments& "$javainput";
%typemap(javaout) const OpArguments& {
  return $jnicall;
}
%typemap(javadirectorin) const OpArguments& "$jniinput";
%typemap(javadirectorout) const OpArguments& "$javacall";
%typemap(directorin,descriptor="Lcom/opera/android/op/OpArguments;") const OpArguments& %{
  $input = $1.GetObject(jenv, "com/opera/android/op");
%}

%typemap(in) const OpArguments& %{
  if ($input && jenv->IsSameObject($input, NULL) == JNI_FALSE) {
    static jfieldID swigCPtr_id = 0;
    if (!swigCPtr_id) {
      swigCPtr_id = jenv->GetFieldID(
          jenv->FindClass("com/opera/android/op/OpArguments"), "swigCPtr", "J");
    }
    $1 = ((OpArguments*)jenv->GetLongField($input, swigCPtr_id))->SetObject(jenv, $input);
  } else {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "OpArguments const & reference is null");
    return;
  }
%}

%typemap(jni) base::Callback<void(const OpArguments&)> "jobject";
%typemap(jtype) base::Callback<void(const OpArguments&)> "OpCallback";
%typemap(jstype) base::Callback<void(const OpArguments&)> "OpCallback";
%typemap(javain) base::Callback<void(const OpArguments&)> "$javainput";

%typemap(in) base::Callback<void(const OpArguments&)> %{
  if ($input && jenv->IsSameObject($input, NULL) == JNI_FALSE) {
    base::android::ScopedJavaGlobalRef<$typemap(jni, $1_type)> $input_ref;
    $input_ref.Reset(jenv, $input);
    static jfieldID swigCPtr_id = 0;
    if (!swigCPtr_id) {
      swigCPtr_id = jenv->GetFieldID(
          jenv->FindClass("com/opera/android/op/OpCallback"), "swigCPtr", "J");
    }
    $1 = base::Bind(
        &RunOpCallback,
        base::Unretained((OpCallback*)jenv->GetLongField($input, swigCPtr_id)),
        $input_ref);
  } else {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "OpCallback null");
    return $null;
  }
%}

%typemap(javadirectorin) base::Callback<void(const OpArguments&)> "$jniinput";
%typemap(directorin,descriptor="Lcom/opera/android/op/OpCallback;") base::Callback<void(const OpArguments&)> %{
  jclass $1_clazz = jenv->FindClass("com/opera/android/op/OpCallable");

  base::android::ScopedJavaGlobalRef<jobject> $1_ref;
  $1_ref.Reset(jenv, jenv->NewObject(
      $1_clazz, jenv->GetMethodID($1_clazz, "<init>", "(JZ)V"),
      reinterpret_cast<jlong>(new OpCallable($1)), true));

  $input = $1_ref.obj();
%}

%ignore RunOpCallback;
%feature("director", assumeoverride=1) OpCallback;
%inline {
class OpCallback {
 public:
  virtual ~OpCallback() {}
  virtual void Run(const OpArguments& args) = 0;
};

static void RunOpCallback(
    OpCallback* callback,
    const base::android::ScopedJavaGlobalRef<jobject>& callback_ref,
    const OpArguments& arguments) {
  callback->Run(arguments);
}

class OpCallable : public OpCallback {
 public:
  virtual ~OpCallable() {}
  OpCallable(OpRunnable runnable) : runnable_(runnable) {}
  virtual void Run(const OpArguments& args) {
    runnable_.Run(args);
  }
 private:
  OpRunnable runnable_;
};
}
