// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_H_
#define MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_H_

#include <list>
#include <vector>
#include <string>

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"

namespace opera {

class BookmarkRootFolder;
class BookmarkFolder;
class Bookmark;

class Bookmark {
 public:
  typedef std::vector<base::string16> Keywords;

  enum Type {
    BOOKMARK_SITE,
    BOOKMARK_FOLDER,
  };

  Bookmark(const base::string16& name, const std::string& url, const std::string& guid);
  virtual ~Bookmark();

  /**
   * @return the title of the bookmark.
   */
  const base::string16& title() const { return title_; }

  /**
   * @return the URL of the bookmark (empty for folders).
   */
  const std::string& url() const {
    return url_;
  }

  /**
   * @return the GUID.
   */
  const std::string& guid() const { return guid_; }

  /**
   * @return the parent folder or NULL if this is a root folder.
   */
  BookmarkFolder* parent() const { return parent_; }

  /**
   * Get the Bookmark type.
   */
  virtual Bookmark::Type GetType() const = 0;

  /**
   * Get the key words for this favorite.
   */
  virtual Keywords GetKeyWords() const;

  /**
   * Whether this favorite's URL should be included for suggestions and
   * used by the "add to speed dial" icon.
   */
  virtual bool AllowURLIndexing() const = 0;

  virtual void SetUrl(const std::string& url, bool by_user_action);

  virtual void SetTitle(const base::string16& title, bool by_user_action);

 private:
  friend class BookmarkFolder;
  friend class BookmarkRootFolder;
  friend class BookmarkSite;

  void set_parent(BookmarkFolder* parent);

  base::string16 title_;
  std::string url_;

  /**
   * Bookmark unique id. Uses for synchronisation purposes.
   * For bookmark extension it is extension's id generated
   * from its public key.
   */
  std::string guid_;

  /**
   * Bookmark parent folder.
   */
  BookmarkFolder* parent_;
};

/**
 * Bookmark Site
 * A Bookmark Site represented by an URL shown in UI with thumbnail
 * representation.
 */
class BookmarkSite : public Bookmark {
 public:
  explicit BookmarkSite(const base::string16& name,
                        const std::string& url,
                        const std::string& guid);

  // Bookmark overrides:
  Bookmark::Type GetType() const override;
  bool AllowURLIndexing() const override;
};

/**
 * A Folder grouping a set of bookmarks in the UI.
 */
class BookmarkFolder : public Bookmark {
 public:
  BookmarkFolder(const base::string16& name,
                 const std::string& guid);
  virtual ~BookmarkFolder();

  /**
   * @return number of children of this BookmarkFolder.
   */
  int GetChildCount() const;

  /**
   * @return the child at index @p index, if any.
   */
  Bookmark* GetChildByIndex(int index) const;

  /**
   * Get the index of given bookmark.
   * @return index of bookmark or -1 if such bookmark does not exist in
   *         this BookmarkFolder.
   */
  int GetIndexOf(const Bookmark* bookmark) const;

  /**
   * Find the Bookmark in the current folder or any of its subfolders.
   *
   * @return The found Bookmark or NULL.
   */
  Bookmark* FindByGUID(const std::string& guid) const;

  // Bookmark overrides:
  Bookmark::Type GetType() const override;
  bool AllowURLIndexing() const override;

 protected:
  friend class BookmarkRootFolder;

  /**
   * Add child at given index.
   *
   * The index must be in the [0, child count] interval.
   */
  void AddChildAt(Bookmark* child, int index);

  /**
   * Remove child bookmark.
   */
  void RemoveChild(Bookmark* child);

  /**
   * Move child to another BookmarkFolder.
   */
  void MoveChildToFolder(Bookmark* child,
                         BookmarkFolder* dest_folder,
                         int src_index,
                         int dest_index);

  // Is this a root folder?
  virtual bool IsRootFolder() const;

 private:
  typedef std::list<Bookmark*> BookmarkEntries;

  /**
   * Child bookmarks.
   */
  BookmarkEntries children_;
};


}  // namespace opera

#endif  // MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_H_
