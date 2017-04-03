// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SWIG_UTILS_OP_ARGUMENTS_H_
#define COMMON_SWIG_UTILS_OP_ARGUMENTS_H_

#include <jni.h>
#include <string>

#include "base/android/scoped_java_ref.h"

// -----------------------------------------------------------------------------
// Introduction
// -----------------------------------------------------------------------------
//
// The Java class OpCallback is a specific function object that using
// SWIG typemaps map to the template instance
// base::Callback<void(const OpArguments&)>. With the
// OpCallback/base::Callback<void(const OpArguments&)> it is possible
// to create SWIG interfaces taking base::Callback<void(const
// OpArguments&)> as C++ function arguments and pass OpCallback Java
// function arguments. To be able to pass base::Callback<void(const
// OpArguments&)> to Java (and those callbacks being callable from
// Java), the OpCallable class is used to implement the OpCallback
// abstract class.
//
// -----------------------------------------------------------------------------
// Calling Java callbacks from C++
// -----------------------------------------------------------------------------
//
// OpCallback is declared elsewhere (implicitly through inlined SWIG
// C++ code), but the resulting Java class is roughly:
//
// package com.opera.android.op;
//
// abstract public class OpCallback {
//   ...
//   public abstract void Run(OpArguments args);
//   ...
// }
//
// The arguments object is owned by the caller and must not be retained by the
// callback implementation. The caller is free to destroy the object upon
// return in order to release resources such as JNI refs that it might hold.
//
// Passing a callback to C++ requires a Java callback object to be
// created, and that callback object needs to inherit from OpCallback
// and thus needs to implement the Run abstract method.
//
// Example:
//
// OpCallback cb = new OpCallback() {
//   @Override
//   void Run(OpArguments arguments) {
//     Log.d("OpCallback", "Callbacks are really cool!");
//   }
// };
//
// This object can now be passed to functions taking
// base::Callback<void(const OpArguments&)>.
//
// -----------------------------------------------------------------------------
// Calling C++ callbacks from Java
// -----------------------------------------------------------------------------
//
// Calling callbacks from Java first requires that there is a director
// present taking an OpCallback argument on the Java side and an
// OpRunnable argument on the C++ side. Roughly what you'd get by
// setting up a SWIG interface like so:
//
// %feature("director", assumeoverride=1) Foo;
// class Foo {
//  public:
//   virtual void foo(OpRunnable callback) = 0;
// };
//
// The corresponding Java implementation of foo would then be:
//
// public void foo(OpCallback callback) {
//   OpArguments args = new OpArguments();
//   callback.Run(args);
//   args.delete();
// }
//
// Each OpArguments instance creates at least one JNI ref, so when the object
// is owned by Java code you must use delete() to explicitly release it unless
// you can guarantee a bounded small number of instances created. C++-owned
// OpArguments will have their JNI refs released by the destructor.
//
// The arguments object is owned by the caller, and you may safely assume that
// it will not be retained by the callback.
//
// -----------------------------------------------------------------------------
// Passing arguments to callbacks
// -----------------------------------------------------------------------------
//
// Argument handling needs some special care since it is tricky to
// accommodate template classes passing the SWIG barrier. This is
// solved using inheritance and the const OpArguments& object. As
// probably noted already the C++ callback takes an argument of type
// const OpArguments&. This type has also a corresponding SWIG
// interface that enables passing it over the language barriers but in
// the end re-claiming some degree of type-safety.
//
// Example:
//
// Given the the C++ function with its SWIG interface:
//
// void foo(base::Callback<void(const OpArguments&)> callback,
//          const OpArguments& arguments) {
//   callback.Run(arguments);
// }
//
// we can in Java do:
//
// class MyArguments extends OpArguments {
//   public String myString;
// }
//
// OpCallback cb = new OpCallback() {
//   @Override
//   void Run(const OpArguments& arguments) {
//     if (arguments instanceof MyArguments) {
//       Log.d("OpCallback", ((MyArguments) arguments).myString);
//     } else {
//       Log.d("OpCallback", "And this shouldn't happen!");
//     }
//   }
// };
//
// MyArguments args = new MyArguments();
// args.myString = "Callbacks are cooler than I first expected!";
// foo(cb, args);

// And with the help of SWIG, we needn't even create the arguments
// object from Java, but can rely on the proxy object creation when
// calling from C++.
//
// Example:
//
// class MyArguments : public OpArguments {
//  public:
//   std::string myString;
//  private:
//   SWIG_CLASS_NAME
// };
//
// void foo(base::Callback<void(const OpArguments&)> callback) {
//   MyArguments args;
//   args.myString = std::string("It's amazing!");
//   callback(args);
// }
//
// foo(cb);
//
// When declaring a class with a const OpArguments& base class, it
// must be possible to look up the corresponding SWIG proxy name. This
// is accomplished by just tagging the derived class with
// SWIG_CLASS_NAME in the private section. Without this and instanceof
// on the Java side will only ever see the OpArguments type. With
// SWIG_CLASS_NAME added, a MyArguments type will be available.
//
// N.B. When passing arguments to a C++ callback called from Java you
// need to perform a static cast to be able to extract the contents of
// the arguments object, completely lacking any imposed safety nets
// from features similar to Java's instanceof. That's just the way it
// is.

#define SWIG_CLASS_NAME virtual std::string pretty_function() const {   \
      return std::string(__PRETTY_FUNCTION__); \
  }

class OpArguments {
 public:
  OpArguments() {}
  virtual ~OpArguments();

  jobject GetObject(JNIEnv* jenv, const std::string& package_name) const;
  OpArguments* SetObject(JNIEnv* jenv, jobject object);

 private:
  SWIG_CLASS_NAME
  std::string GetClassName() const;

  mutable base::android::ScopedJavaGlobalRef<jobject> object_;
};

#endif  // COMMON_SWIG_UTILS_OP_ARGUMENTS_H_
