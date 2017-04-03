// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/ui/tabs/thumbnail_store.h"

#include <dirent.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdint>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

#include "chill/browser/ui/tabs/thumbnail_cache.h"
#include "common/opengl_utils/gl_mipmap.h"

namespace opera {
namespace tabui {

class ThumbnailStore::Impl {
 public:
  Impl(const ThumbnailCache& cache,
       bool is_rtl,
       const std::string& file_path,
       const std::string& file_prefix,
       const std::string& legacy_file_prefix = "")
      : cache_(cache),
        is_rtl_(is_rtl),
        file_path_(file_path),
        file_prefix_(file_prefix),
        legacy_file_prefix_(legacy_file_prefix),
        reduce_levels_(display::Screen::GetScreen()
                                      ->GetPrimaryDisplay()
                                      .device_scale_factor() >= 2
                           ? 1
                           : 0),
        restore_(false),
        destroyed_(false),
        worker_thread_(&Impl::WorkerMain, this) {}

  ~Impl() {
    // Notify worker thread and wait for it to terminate.
    {
      std::unique_lock<std::mutex> scoped_lock(mutex_);
      destroyed_ = true;
      cv_.notify_one();
    }
    worker_thread_.join();
  }

  void AddTab(int id) {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    added_tabs_.push_back(id);
  }

  void AddTab(int id, const std::string& legacy_file_suffix) {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    DCHECK(!legacy_file_prefix_.empty());
    DCHECK(!legacy_file_suffix.empty());
    added_tabs_.push_back(id);
    legacy_suffix_map_.emplace(legacy_file_suffix, id);
  }

  void RemoveTab(int id) {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    removed_tabs_.push_back(id);
  }

  void Restore() {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    restore_ = true;
    // Kick worker thread so that it runs a pass immediately.
    cv_.notify_one();
  }

 private:
  void WorkerMain() {
    int64_t generation = 0;
    std::unordered_set<int> known_tabs;

    while (true) {
      std::vector<int> added_tabs;
      std::vector<int> removed_tabs;
      bool restore = false;

      // TODO(ollel): Remove when support for old file format is dropped.
      std::unordered_multimap<std::string, int> legacy_suffix_map;

      {
        std::unique_lock<std::mutex> scoped_lock(mutex_);
        std::swap(added_tabs, added_tabs_);
        std::swap(removed_tabs, removed_tabs_);
        std::swap(restore, restore_);
        std::swap(legacy_suffix_map, legacy_suffix_map_);
      }

      // Tabs added since last pass.
      known_tabs.insert(added_tabs.begin(), added_tabs.end());

      // Tabs removed since last pass.
      for (int id : removed_tabs) {
        known_tabs.erase(id);
        unlink(FileName(id).c_str());
      }

      // Restore thumbnails from file.
      if (restore) {
        // Pattern that matches stored thumbnail files.
        std::string pattern = file_prefix_ + "%d";

        // List of files that should be erased.
        std::vector<std::string> erase_list;

        // TODO(ollel): Remove when support for old file format is dropped.
        std::unordered_set<std::string> legacy_file_suffixes;

        erase_list.push_back(TmpFileName());

        DIR* dir = opendir(file_path_.c_str());
        if (!dir) {
          LOG(FATAL) << "Could not open thumbnail directory: \"" << file_path_
                     << "\"";
        }

        for (dirent* entry = readdir(dir); entry; entry = readdir(dir)) {
          if (entry->d_type != DT_REG)
            continue;

          int id;

          // Erase files with naming used by old tab ui.
          if (!legacy_file_prefix_.empty() &&
              !legacy_file_prefix_.compare(0, std::string::npos, entry->d_name,
                                           legacy_file_prefix_.size())) {
            std::string suffix(entry->d_name + legacy_file_prefix_.size());
            if (legacy_suffix_map.count(suffix))
              legacy_file_suffixes.insert(suffix);
            erase_list.push_back(file_path_ + '/' + entry->d_name);
            continue;
          }

          // Skip file if not a thumbnail.
          if (sscanf(entry->d_name, pattern.c_str(), &id) != 1)
            continue;

          std::string file_name = file_path_ + '/' + entry->d_name;

          // Remove thumbnail files that should not be restored.
          if (!known_tabs.count(id)) {
            erase_list.push_back(file_name);
            continue;
          }

          std::shared_ptr<GLMipMap> mipmap(
              GLMipMap::CreateFromFile(file_name, is_rtl_));
          int64_t generation;
          if (!mipmap || (generation = cache_.AddMipmap(id, mipmap)) == -1) {
            erase_list.push_back(file_name);
            continue;
          }
        }

        // Restore remaining thumbnails from old file format if available.
        // This is a one-time conversion. The old files are deleted and replaced
        // by files in the new format.
        // TODO(ollel): Remove when support for old file format is dropped.
        for (const std::string& suffix : legacy_file_suffixes) {
          std::string file_name =
              file_path_ + '/' + legacy_file_prefix_ + suffix;
          std::shared_ptr<GLMipMap> mipmap =
              GLMipMap::CreateFromLegacyFile(file_name);
          if (!mipmap)
            continue;
          // The old suffix is based on the url and we could have more than one
          // tab visiting the same url.
          auto range = legacy_suffix_map.equal_range(suffix);
          for (auto it = range.first; it != range.second; ++it) {
            if (!cache_.GetMipmap(it->second, false))
              cache_.AddMipmap(it->second, mipmap);
          }
        }

        for (auto file_name : erase_list)
          unlink(file_name.c_str());

        closedir(dir);
      }

      // Write updated thumbnails to disk.
      for (std::pair<int64_t, int> p : cache_.GetChangedTabIds(generation)) {
        generation = p.first;
        int id = p.second;
        if (!known_tabs.count(id))
          continue;
        auto mipmap = cache_.GetMipmap(id, false);
        if (mipmap) {
          auto reduced =
              mipmap->CreateReduced(mipmap->original_levels() - reduce_levels_);
          if (reduced->SaveToFile(TmpFileName(), is_rtl_))
            rename(TmpFileName().c_str(), FileName(id).c_str());
          else
            unlink(TmpFileName().c_str());
        } else {
          // If we didn't get a cached thumbnail it could be because the last
          // readback failed or was removed by some other reason. Any stored
          // thumbnail is then obsoleted and needs to be removed to prevent it
          // from getting loaded in a session restore.
          unlink(FileName(id).c_str());
        }
      }

      // Wait a second before running next pass or terminate thread if
      // destructor is pending.
      std::unique_lock<std::mutex> scoped_lock(mutex_);
      cv_.wait_for(scoped_lock, std::chrono::seconds(1));
      if (destroyed_)
        break;
    }
  }

  std::string FileName(int tab_id) const {
    std::stringstream ss;
    ss << file_path_ << '/' << file_prefix_ << tab_id;
    return ss.str();
  }

  std::string TmpFileName() const {
    return file_path_ + "/" + file_prefix_ + "__temporary_file__";
  }

  // Guards access to data members.
  std::mutex mutex_;

  // In-memory cache for thumbnails.
  ThumbnailCache cache_;

  // If thumbnails are in rtl mode.
  const bool is_rtl_;

  // Location of thumbnails on disk.
  const std::string file_path_;

  // Prefix that will be added to file names.
  const std::string file_prefix_;

  // File name prefix of thumbnail files stored in old format.
  const std::string legacy_file_prefix_;

  // TODO(ollel): Remove when support for old file format is dropped.
  std::unordered_multimap<std::string, int> legacy_suffix_map_;

  // Number of mip levels to skip when storing on disk.
  const size_t reduce_levels_;

  // Tabs added since last worker pass.
  std::vector<int> added_tabs_;

  // Tabs removed since last worker pass.
  std::vector<int> removed_tabs_;

  // Set when the worker thread should restore tabs at next wake-up.
  bool restore_;

  // Set when the worker thread should terminate at next wake-up.
  bool destroyed_;

  // Set when synchronization is requested.
  bool sync_requested_;

  // For wake-up calls to worker thread.
  std::condition_variable cv_;

  // Worker thread.
  std::thread worker_thread_;
};

ThumbnailStore::ThumbnailStore(const ThumbnailCache& cache,
                               bool is_rtl,
                               const std::string& file_path,
                               const std::string& file_prefix)
    : impl_(new Impl(cache, is_rtl, file_path, file_prefix)) {}

ThumbnailStore::ThumbnailStore(const ThumbnailCache& cache,
                               bool is_rtl,
                               const std::string& file_path,
                               const std::string& file_prefix,
                               const std::string& legacy_file_prefix)
    : impl_(
          new Impl(cache, is_rtl, file_path, file_prefix, legacy_file_prefix)) {
}

void ThumbnailStore::AddTab(int id) {
  impl_->AddTab(id);
}

void ThumbnailStore::AddTab(int id, const std::string& legacy_file_suffix) {
  impl_->AddTab(id, legacy_file_suffix);
}

void ThumbnailStore::RemoveTab(int id) {
  impl_->RemoveTab(id);
}

void ThumbnailStore::Restore() {
  impl_->Restore();
}

}  // namespace tabui
}  // namespace opera
