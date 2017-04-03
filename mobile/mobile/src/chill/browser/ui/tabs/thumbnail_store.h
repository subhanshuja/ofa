// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_UI_TABS_THUMBNAIL_STORE_H_
#define CHILL_BROWSER_UI_TABS_THUMBNAIL_STORE_H_

#include <memory>
#include <string>

#include "chill/browser/ui/tabs/thumbnail_store.h"

namespace opera {
namespace tabui {

class ThumbnailCache;

// On-disk cache for tab thumbnails.
//
// The class is implemented as an opaque pointer to the real implementaion,
// allowing it to be passed by value between C++ and Java.
class ThumbnailStore {
  ThumbnailStore& operator=(const ThumbnailStore&) = delete;

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

  // Adds a tab to the set that will get thumbnails stored on file.
  void AddTab(int id);

  // Adds a tab to the set that will get thumbnails stored on file.
  // This version will also try to restore from the old file format.
  // TODO(ollel): Remove when support for old file format is dropped.
  void AddTab(int id, const std::string& legacy_file_suffix);

  // Removed a tab from the set that will get thumbnails stored on file. The
  // thumbnail file will be removed too.
  void RemoveTab(int id);

  // Restore saved thumbnails. Must be called after all known tabs have been
  // added since unknown files will be cleaned up.
  void Restore();

 private:
  class Impl;

  std::shared_ptr<Impl> impl_;
};

}  // namespace tabui
}  // namespace opera

#endif  // CHILL_BROWSER_UI_TABS_THUMBNAIL_STORE_H_
