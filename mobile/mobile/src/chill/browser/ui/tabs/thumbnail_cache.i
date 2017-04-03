// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/ui/tabs/thumbnail_cache.h"
%}

%typemap(jstype) jobject bitmap "android.graphics.Bitmap"
%typemap(jtype) jobject bitmap "android.graphics.Bitmap"

namespace opera {
namespace tabui {

// Apply mixedCase to method names in Java.
%rename("%(regex:/ThumbnailCache::([A-Z].*)/\\l\\1/)s", regextarget=1, fullname=1, %$isfunction) ".*";

class ThumbnailCache {
 public:
  ThumbnailCache();
  ThumbnailCache(jobject bitmap);

  void AddTab(int id);
  void RemoveTab(int id);
  void Clear();
  void Clear(int id);
};

%extend ThumbnailCache {
  bool hasBitmap(int id) {
    return $self->GetMipmap(id, false) != nullptr;
  }
};

}  // namespace tabui
}  // namespace opera
