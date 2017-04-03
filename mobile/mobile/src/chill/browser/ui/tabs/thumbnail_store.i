// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/ui/tabs/thumbnail_store.h"
%}

namespace opera {
namespace tabui {

// Apply mixedCase to method names in Java.
%rename("%(regex:/ThumbnailStore::([A-Z].*)/\\l\\1/)s", regextarget=1, fullname=1, %$isfunction) ".*";

class ThumbnailStore {
 public:
  ThumbnailStore(const ThumbnailCache& cache,
                 bool is_rtl,
                 const std::string& file_path,
                 const std::string& file_prefix);

  ThumbnailStore(const ThumbnailCache& cache,
                 bool is_rtl,
                 const std::string& file_path,
                 const std::string& file_prefix,
                 const std::string& legacy_file_prefix);

  void AddTab(int id);

  // TODO(ollel): Remove when support for old file format is dropped.
  void AddTab(int id, const std::string& legacy_file_suffix);

  void RemoveTab(int id);
  void Restore();
};

}  // namespace tabui
}  // namespace opera
