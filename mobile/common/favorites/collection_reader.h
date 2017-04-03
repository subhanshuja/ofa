// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_COLLECTION_READER_H_
#define MOBILE_COMMON_FAVORITES_COLLECTION_READER_H_

#include "base/memory/ref_counted.h"

namespace opera {
class FavoriteCollection;
class FavoriteCollectionObserver;
class FavoriteFolder;
}

namespace mobile {

class FavoritesImpl;
class FavoritesObserver;
class Folder;

class CollectionReader {
 public:
  class Observer {
   public:
    virtual ~Observer();

    virtual void CollectionReaderIsFinished() = 0;

   protected:
    Observer();
  };

  CollectionReader(FavoritesImpl* favorites,
                   const scoped_refptr<opera::FavoriteCollection>& collection,
                   Observer* observer);
  virtual ~CollectionReader();

  bool IsFinished() const;

 private:
  void Import();
  void Import(Folder* dst, opera::FavoriteFolder* src);
  void CheckState();

  FavoritesImpl* favorites_;
  Observer* observer_;
  bool finished_;
  scoped_refptr<opera::FavoriteCollection> collection_;
  std::unique_ptr<FavoritesObserver> favorites_observer_;
  std::unique_ptr<opera::FavoriteCollectionObserver> collection_observer_;

  DISALLOW_COPY_AND_ASSIGN(CollectionReader);
};

}  // namespace mobile

#endif  // MOBILE_COMMON_FAVORITES_COLLECTION_READER_H_
