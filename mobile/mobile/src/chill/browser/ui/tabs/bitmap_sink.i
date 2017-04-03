// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/ui/tabs/bitmap_sink.h"
%}

%typemap(jstype) jobject bitmap "android.graphics.Bitmap"
%typemap(jtype) jobject bitmap "android.graphics.Bitmap"

namespace opera {
namespace tabui {

%typemap(javainterfaces) BitmapSink "com.opera.api.BitmapSink"
%nodefaultctor BitmapSink;
%rename(NativeBitmapSink) BitmapSink;

// Apply mixedCase to method names in Java.
%rename("%(regex:/BitmapSink::([A-Z].*)/\\l\\1/)s", regextarget=1, fullname=1, %$isfunction) ".*";

class BitmapSink {
 public:
  void Fail();
  void Accept(jobject bitmap);
};

}  // namespace tabui
}  // namespace opera
