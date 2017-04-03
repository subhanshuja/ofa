// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "common/old_bookmarks/bookmark.h"
#include "common/old_bookmarks/bookmark_collection.h"
#include "common/old_bookmarks/bookmark_collection_observer.h"
#include "chill/browser/old_bookmark_collection.h"
%}

namespace opera {

%rename (OldBookmarkCollection) BookmarkCollection;
%rename (OldBookmarkCollectionObserver) BookmarkCollectionObserver;
%rename (OldBookmark) Bookmark;
%rename (OldBookmarkFolder) BookmarkFolder;
%rename (OldBookmarkSite) BookmarkSite;

%typemap(javacode) Bookmark %{
  static OldBookmark CreateFromCPtr(long cPtr, boolean cMemoryOwn) {
    if (cPtr == 0)
      return null;
    if (CPtrIsFolder(cPtr))
      return new OldBookmarkFolder(cPtr, cMemoryOwn);
    else
      return new OldBookmarkSite(cPtr, cMemoryOwn);
  }
%}

%typemap(javaout) Bookmark* {
    return OldBookmark.CreateFromCPtr($jnicall, $owner);
  }

%typemap(javadirectorin) Bookmark* "OldBookmark.CreateFromCPtr($jniinput, false)"

class BookmarkFolder;

%nodefaultctor Bookmark;
%nodefaultdtor Bookmark;
class Bookmark {
 public:
  const base::string16& title() const;
  const std::string& guid() const;
  BookmarkFolder* parent() const;
};

%nodefaultctor BookmarkSite;
%nodefaultdtor BookmarkSite;
class BookmarkSite : public Bookmark {
 public:
  const std::string& url() const;
};

%nodefaultctor BookmarkFolder;
%nodefaultdtor BookmarkFolder;
class BookmarkFolder : public Bookmark {
 public:
  int GetChildCount() const;
  Bookmark* GetChildByIndex(int index) const;
  int GetIndexOf(const Bookmark* bookmark) const;
};

%extend Bookmark {
  static bool CPtrIsFolder(jlong cPtr) {
    opera::Bookmark* bookmark = (opera::Bookmark*) cPtr;
    return bookmark->GetType() == opera::Bookmark::BOOKMARK_FOLDER;
  }
};

class BookmarkCollectionObserver;

%nodefaultctor BookmarkCollection;
%nodefaultdtor BookmarkCollection;
class BookmarkCollection {
 public:
  enum ChangeReason {
    CHANGE_REASON_SYNC,
    CHANGE_REASON_USER,
    CHANGE_REASON_AUTOUPDATE,
    CHANGE_REASON_OTHER
  };

  virtual void AddObserver(BookmarkCollectionObserver* observer);
  virtual void RemoveObserver(BookmarkCollectionObserver* observer);
  virtual bool IsLoaded() const;
  virtual BookmarkFolder* GetRootFolder() const;

  virtual void RemoveBookmark(Bookmark* bookmark, ChangeReason reason);
};

%feature("director", assumeoverride=1) BookmarkCollectionObserver;
class BookmarkCollectionObserver {
 public:
  virtual ~BookmarkCollectionObserver();
  virtual void BookmarkCollectionLoaded() = 0;
  virtual void BookmarkCollectionDeleted() = 0;
  virtual void BookmarkAdded(const Bookmark* bookmark,
                             int new_index,
                             BookmarkCollection::ChangeReason reason) = 0;
  virtual void BookmarkRemoved(const Bookmark* bookmark,
                               const BookmarkFolder* parent,
                               int index,
                               BookmarkCollection::ChangeReason reason) = 0;
  virtual void BookmarkMoved(const Bookmark* bookmark,
                             const BookmarkFolder* old_parent,
                             int old_index,
                             const BookmarkFolder* new_parent,
                             int new_index,
                             BookmarkCollection::ChangeReason reason) = 0;
  virtual void BookmarkChanged(const Bookmark* bookmark,
                               BookmarkCollection::ChangeReason reason) = 0;
};

%nodefaultctor OldBookmarkCollectionContainer;
class OldBookmarkCollectionContainer {
 public:
  BookmarkCollection* GetCollection();
};

%newobject OpenOldBookmarkCollection;
static OldBookmarkCollectionContainer* OpenOldBookmarkCollection();

}  // namespace opera
