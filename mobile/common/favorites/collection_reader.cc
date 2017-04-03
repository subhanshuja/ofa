// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/favorites/collection_reader.h"

#include <string>

#include "base/callback.h"

#include "common/favorites/favorite_collection.h"
#include "common/favorites/favorite_collection_observer.h"

#include "mobile/common/favorites/favorites_impl.h"
#include "mobile/common/favorites/favorites_observer.h"
#include "mobile/common/favorites/folder.h"

namespace {

mobile::Favorite* FindInFolderByGUID(mobile::Folder* folder,
                                     const std::string& guid) {
  for (int i = 0; i < folder->Size(); ++i) {
    mobile::Favorite* child = folder->Child(i);
    if (child->guid() == guid) {
      return child;
    }
  }
  return NULL;
}

class ReadyFavoritesObserver : public mobile::FavoritesObserver {
 public:
  ReadyFavoritesObserver(const base::Closure& callback)
      : callback_(callback) {
  }

  void OnReady() override {
    callback_.Run();
  }

  void OnLoaded() override {
  }

  void OnAdded(int64_t id, int64_t parent, int pos) override {
  }

  void OnRemoved(int64_t id,
                 int64_t old_parent,
                 int old_pos,
                 bool recursive) override {
  }

  void OnMoved(int64_t id,
               int64_t old_parent,
               int old_pos,
               int64_t new_parent,
               int new_pos) override {
  }

  void OnChanged(int64_t id, int64_t parent, unsigned int changed) override {
  }

 private:
  base::Closure callback_;
};

class LoadedCollectionObserver : public opera::FavoriteCollectionObserver {
 public:
  LoadedCollectionObserver(const base::Closure& callback)
      : callback_(callback) {
  }

  void FavoriteCollectionLoaded() override {
    callback_.Run();
  }

 private:
  base::Closure callback_;
};

}  // namespace

namespace mobile {

CollectionReader::Observer::~Observer() {
}

CollectionReader::Observer::Observer() {
}

CollectionReader::CollectionReader(
    FavoritesImpl* favorites,
    const scoped_refptr<opera::FavoriteCollection>& collection,
    Observer* observer)
    : favorites_(favorites),
      observer_(observer),
      finished_(false),
      collection_(collection) {
  if (collection_->IsLoaded() && favorites_->IsReady()) {
    Import();
  } else {
    base::Closure callback = base::Bind(&CollectionReader::CheckState,
                                        base::Unretained(this));
    if (!favorites_->IsReady()) {
      favorites_observer_.reset(new ReadyFavoritesObserver(callback));
      favorites_->AddObserver(favorites_observer_.get());
    }
    if (!collection_->IsLoaded()) {
      collection_observer_.reset(new LoadedCollectionObserver(callback));
      collection_->AddObserver(collection_observer_.get());
    }
  }
}

CollectionReader::~CollectionReader() {
  if (collection_observer_) {
    collection_->RemoveObserver(collection_observer_.get());
  }
  if (favorites_observer_) {
    favorites_->RemoveObserver(favorites_observer_.get());
  }
}

void CollectionReader::CheckState() {
  if (collection_->IsLoaded() && favorites_->IsReady()) {
    Import();
  }
}

bool CollectionReader::IsFinished() const {
  return finished_;
}

void CollectionReader::Import() {
  Import(favorites_->local_root(), collection_->GetRoot());
  finished_ = true;
  if (observer_) {
    observer_->CollectionReaderIsFinished();
  }
}

void CollectionReader::Import(Folder* dst, opera::FavoriteFolder* src) {
  const bool dst_was_empty = dst->Size() == 0;
  for (auto src_iter(src->begin()); src_iter != src->end(); ++src_iter) {
    Favorite* dst_child;
    opera::Favorite* const src_child = *src_iter;
    switch (src_child->GetType()) {
      case opera::Favorite::kFavoriteFolder:
        dst_child =
            dst_was_empty ? NULL : FindInFolderByGUID(dst, src_child->guid());
        if (!dst_child) {
          dst_child =
              favorites_->CreateFolder(dst->Size(),
                                       src_child->title(),
                                       src_child->guid(),
                                       src_child->pushed_partner_group_id());
        }
        // dst_child might be NULL here when restoring folders that are no
        // longer stored as "real" folders, ie bookmarks and saved pages are
        // removed directly when CreateFolder is called and CreateFolder
        // then returns NULL
        if (dst_child && dst_child->IsFolder()) {
          Import(static_cast<Folder*>(dst_child),
                 static_cast<opera::FavoriteFolder*>(src_child));
        }
        break;
      case opera::Favorite::kFavoriteSite:
        dst_child =
            dst_was_empty ? NULL : FindInFolderByGUID(dst, src_child->guid());
        if (!dst_child) {
          if (src_child->pushed_partner_channel()) {
            dst_child = favorites_->CreatePartnerContent(
                dst,
                dst->Size(),
                src_child->title(),
                src_child->navigate_url(),
                src_child->guid(),
                src_child->pushed_partner_activation_count(),
                src_child->pushed_partner_channel(),
                src_child->pushed_partner_id(),
                src_child->pushed_partner_checksum(),
                true);
          } else {
            dst_child = favorites_->CreateFavorite(dst,
                                                   dst->Size(),
                                                   src_child->title(),
                                                   src_child->navigate_url(),
                                                   src_child->guid(),
                                                   true);
          }
        }
        break;
      case opera::Favorite::kFavoriteExtension:
        continue;
    }
  }
}

}  // namespace mobile
