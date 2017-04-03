// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/ui/tabs/thumbnail_feeder.h"
%}

%typemap(jstype) jobject handler "android.os.Handler"
%typemap(jtype) jobject handler "android.os.Handler"
%typemap(jstype) jobject runnable "java.lang.Runnable"
%typemap(jtype) jobject runnable "java.lang.Runnable"

namespace opera {
namespace tabui {

%nodefaultctor ThumbnailFeeder;
%newobject ThumbnailFeeder::CreateBitmapSink;

// Apply mixedCase to method names in Java.
%rename("%(regex:/ThumbnailFeeder::([A-Z].*)/\\l\\1/)s", regextarget=1, fullname=1, %$isfunction) ".*";

class ThumbnailFeeder {
 public:
  ThumbnailFeeder(const opera::tabui::ThumbnailCache& cache);

  BitmapSink* CreateBitmapSink(int tab_id);
  BitmapSink* CreateBitmapSink(int tab_id, jobject runnable, jobject handler);
};

}  // namespace tabui
}  // namespace opera
