// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/old_bookmarks/bookmark_collection_impl.h"

#include <vector>

#include "base/bind.h"
#include "base/location.h"
#include "base/strings/utf_string_conversions.h"

#include "common/old_bookmarks/bookmark_collection_observer.h"
#include "common/old_bookmarks/bookmark_db_storage.h"
#include "common/old_bookmarks/bookmark_utils.h"

namespace {

#ifndef BOOKMARKS_NO_INDEX
void IndexBookmark(opera::BookmarkIndex* index,
                   opera::Bookmark* bookmark,
                   int /* ignored */) {
  index->Add(bookmark);
}
#endif

void PushBookmarkDataToStorage(
    const scoped_refptr<opera::BookmarkStorage::BookmarkRawDataHolder>& holder,
    opera::Bookmark* bookmark,
    int bookmark_index) {
  holder->PushBookmarkRawData(bookmark->guid(),
                              bookmark_index,
                              bookmark->title(),
                              bookmark->url(),
                              bookmark->GetType(),
                              bookmark->parent()->guid());
}

}  // namespace

namespace opera {

const int BookmarkCollection::kIndexPositionLast = -1;

BookmarkCollectionImpl::BookmarkCollectionImpl(
    BookmarkStorage* storage,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner)
  : weak_ptr_factory_(this),
#ifndef BOOKMARKS_NO_INDEX
    index_(new BookmarkIndex),
#endif
    data_store_(storage),
    ui_task_runner_(ui_task_runner),
    is_saving_(false),
    storage_dirty_(false) {
  DCHECK(storage);
  Load();
}

BookmarkCollectionImpl::~BookmarkCollectionImpl() {
  UpdatePersistentStorageOnExit();
}

void BookmarkCollectionImpl::Shutdown() {
  // Browser is shutting down. Cancel any pending save - including those
  // already running on DB thread and inavlidate weak pointers so
  // no scheduled CollectionSaved callback gets invoked. We'll save
  // Bookmarks on exit using blocking I/O operation that cannot be
  // interrupted by shutdown.
  if (!pending_save_.is_null() || is_saving_)
    storage_dirty_ = true;

  pending_save_.Reset();
  weak_ptr_factory_.InvalidateWeakPtrs();

  data_store_->Stop();
  OnCollectionDestroyed();
}

void BookmarkCollectionImpl::AddObserver(BookmarkCollectionObserver* observer) {
  observer_list_.AddObserver(observer);
}

void BookmarkCollectionImpl::RemoveObserver(
    BookmarkCollectionObserver* observer) {
  observer_list_.RemoveObserver(observer);
}

void BookmarkCollectionImpl::Load() {
  data_store_->Load(base::Bind(&BookmarkCollectionImpl::CollectionLoaded,
                               base::Unretained(this)));
}

void BookmarkCollectionImpl::Flush() {
  if (!root_folder_ || (!storage_dirty_ && pending_save_.is_null()))
    return;

  SaveToStorage(false);
  data_store_->Flush();
}

bool BookmarkCollectionImpl::IsLoaded() const {
  return root_folder_ != NULL;
}

BookmarkFolder* BookmarkCollectionImpl::GetRootFolder() const {
  return root_folder_.get();
}

BookmarkSite* BookmarkCollectionImpl::AddBookmarkSite(const std::string& url,
                                                      const base::string16& name,
                                                      BookmarkFolder* parent,
                                                      int index,
                                                      const std::string& guid,
                                                      ChangeReason reason) {
  if (!IsLoaded())
    return NULL;
  if (!guid.empty() && FindByGUID(guid))
    return NULL;

  BookmarkSite* fav = new BookmarkSite(name, url, guid);

  root_folder_->AddChildAt(parent, fav, &index);

  OnBookmarkAdded(fav, index, reason);
  return fav;
}

BookmarkFolder* BookmarkCollectionImpl::AddBookmarkFolder(
    const base::string16& name,
    BookmarkFolder* parent,
    int index,
    const std::string& guid,
    ChangeReason reason) {
  if (!IsLoaded())
    return NULL;
  if (!guid.empty() && FindByGUID(guid))
    return NULL;

  BookmarkFolder* folder = new BookmarkFolder(name, guid);
  root_folder_->AddChildAt(parent, folder, &index);

  OnBookmarkAdded(folder, index, reason);
  return folder;
}

void BookmarkCollectionImpl::RemoveBookmark(Bookmark* bookmark,
                                            ChangeReason reason) {
  DCHECK(bookmark);

  if (!IsLoaded())
    return;

  if (bookmark->GetType() == Bookmark::BOOKMARK_FOLDER) {
    BookmarkFolder* removed_folder = static_cast<BookmarkFolder*>(bookmark);
    while (removed_folder->GetChildCount() > 0) {
      RemoveBookmark(removed_folder->GetChildByIndex(0), reason);
    }
  }

  OnBookmarkRemoved(bookmark,
                    bookmark->parent(),
                    bookmark->parent()->GetIndexOf(bookmark),
                    reason);

  root_folder_->Remove(bookmark);
  UpdatePersistentStorage();
}

bool BookmarkCollectionImpl::MoveBookmark(Bookmark* bookmark,
                                          BookmarkFolder* dest_folder,
                                          int new_index,
                                          ChangeReason reason) {
  if (!IsLoaded())
    return false;

  BookmarkFolder* old_parent = bookmark->parent();
  int old_index = old_parent->GetIndexOf(bookmark);

  root_folder_->Move(bookmark, dest_folder, old_index, new_index);

  OnBookmarkMoved(bookmark,
                  old_parent,
                  old_index,
                  bookmark->parent(),
                  bookmark->parent()->GetIndexOf(bookmark),
                  reason);
  return true;
}

void BookmarkCollectionImpl::RenameBookmark(Bookmark* bookmark,
                                            const base::string16& new_name,
                                            ChangeReason reason) {
  if (!IsLoaded())
    return;

  DCHECK(bookmark);
  bookmark->SetTitle(new_name, reason == CHANGE_REASON_USER);

  OnBookmarkChanged(bookmark, reason);
}

void BookmarkCollectionImpl::ChangeBookmarkURL(Bookmark* bookmark,
                                               const std::string& new_url,
                                               ChangeReason reason) {
  if (!IsLoaded())
    return;

  DCHECK(bookmark && bookmark->GetType() != Bookmark::BOOKMARK_FOLDER);
  RemoveFromOrderedByURLSet(bookmark);
  bookmark->SetUrl(new_url, reason == CHANGE_REASON_USER);
  AddToOrderedByURLSet(bookmark);

  OnBookmarkChanged(bookmark, reason);
}

void BookmarkCollectionImpl::NotifyBookmarkChange(Bookmark* bookmark,
                                                  ChangeReason reason) {
  if (!IsLoaded())
    return;

  DCHECK(bookmark);

  OnBookmarkChanged(bookmark, reason);
}

void BookmarkCollectionImpl::GetBookmarksByURL(
    const std::string& url,
    std::vector<const Bookmark*>* bookmarks) {
  base::AutoLock lock(bookmarks_ordered_by_url_lock_);
  BookmarkSite tmp_node(base::string16(), url, std::string());
  BookmarksOrderedByURLSet::iterator i =
      bookmarks_ordered_by_url_set_.find(&tmp_node);

  while (i != bookmarks_ordered_by_url_set_.end()) {
    if ((*i)->url() != url)
      break;

    bookmarks->push_back(*i);
    ++i;
  }
}

int BookmarkCollectionImpl::GetCount() const {
  if (!IsLoaded())
    return 0;

  return root_folder_->GetChildCount();
}

Bookmark* BookmarkCollectionImpl::GetAt(int index) const {
  if (!IsLoaded())
    return NULL;

  return root_folder_->GetChildByIndex(index);
}

Bookmark* BookmarkCollectionImpl::FindByGUID(const std::string& guid) const {
  if (!IsLoaded())
    return NULL;

  return root_folder_->FindByGUID(guid);
}

void BookmarkCollectionImpl::GetBookmarksWithTitlesMatching(
    const base::string16& query,
    size_t max_count,
    std::vector<BookmarkUtils::TitleMatch>* matches) {
#ifndef BOOKMARKS_NO_INDEX
  index_->GetBookmarksWithTitlesMatching(query, max_count, matches);
#else
  NOTREACHED();
#endif
}

void BookmarkCollectionImpl::PopulateBookmarksByURL(
    const BookmarkRootFolder* root) {
  DCHECK(!bookmarks_ordered_by_url_set_.size()) << "List already populated.";
  base::AutoLock lock(bookmarks_ordered_by_url_lock_);
  PopulateBookmarksByURLForFolder(root);
}

void BookmarkCollectionImpl::PopulateBookmarksByURLForFolder(
    const BookmarkFolder* folder) {
  for (int i = 0; i < folder->GetChildCount(); ++i) {
    const Bookmark* node = folder->GetChildByIndex(i);

    if (node->GetType() == Bookmark::BOOKMARK_FOLDER) {
      PopulateBookmarksByURLForFolder(static_cast<const BookmarkFolder*>(node));
    } else {
      if (!node->url().empty())
        bookmarks_ordered_by_url_set_.insert(node);
    }
  }
}

void BookmarkCollectionImpl::CallBookmarkOnRecreated(BookmarkFolder* root) {
  for (int i = 0; i < root->GetChildCount(); ++i) {
    Bookmark* node = root->GetChildByIndex(i);

    if (node->GetType() == Bookmark::BOOKMARK_FOLDER) {
      BookmarkFolder* folder = static_cast<BookmarkFolder*>(node);
      CallBookmarkOnRecreated(folder);
    }
  }
}

void BookmarkCollectionImpl::OnCollectionLoaded() {
  CallBookmarkOnRecreated(root_folder_.get());
  // PopulateBookmarksByURL must be called after OnRecreated because
  // OnReacreated may modify the Bookmarks' URLs.
  PopulateBookmarksByURL(root_folder_.get());

#ifndef BOOKMARKS_NO_INDEX
  BookmarkUtils::ForEachBookmarkInFolder(
      root_folder_.get(),
      true,
      base::Bind(&IndexBookmark, index_.get()));
#endif

  FOR_EACH_OBSERVER(
      BookmarkCollectionObserver, observer_list_, BookmarkCollectionLoaded());
}

void BookmarkCollectionImpl::OnCollectionDestroyed() {
  FOR_EACH_OBSERVER(
      BookmarkCollectionObserver, observer_list_, BookmarkCollectionDeleted());
}

void BookmarkCollectionImpl::OnBookmarkAdded(const Bookmark* bookmark,
                                             int new_index,
                                             ChangeReason reason) {
#ifndef BOOKMARKS_NO_INDEX
  index_->Add(bookmark);
#endif

  AddToOrderedByURLSet(bookmark);

  FOR_EACH_OBSERVER(BookmarkCollectionObserver,
                    observer_list_,
                    BookmarkAdded(bookmark, new_index, reason));

  UpdatePersistentStorage();
}

void BookmarkCollectionImpl::OnBookmarkRemoved(const Bookmark* bookmark,
                                               const BookmarkFolder* parent,
                                               int index,
                                               ChangeReason reason) {
#ifndef BOOKMARKS_NO_INDEX
  index_->Remove(bookmark);
#endif

  RemoveFromOrderedByURLSet(bookmark);

  FOR_EACH_OBSERVER(BookmarkCollectionObserver,
                    observer_list_,
                    BookmarkRemoved(bookmark, parent, index, reason));
}

void BookmarkCollectionImpl:: OnBookmarkMoved(const Bookmark* bookmark,
                                              const BookmarkFolder* old_parent,
                                              int old_index,
                                              const BookmarkFolder* new_parent,
                                              int new_index,
                                              ChangeReason reason) {
  FOR_EACH_OBSERVER(
      BookmarkCollectionObserver,
      observer_list_,
      BookmarkMoved(
          bookmark, old_parent, old_index, new_parent, new_index, reason));

  UpdatePersistentStorage();
}

void BookmarkCollectionImpl::OnBookmarkChanged(const Bookmark* bookmark,
                                               ChangeReason reason) {
#ifndef BOOKMARKS_NO_INDEX
  index_->Remove(bookmark);
  index_->Add(bookmark);
#endif

  FOR_EACH_OBSERVER(BookmarkCollectionObserver,
                    observer_list_,
                    BookmarkChanged(bookmark, reason));

  UpdatePersistentStorage();
}

void BookmarkCollectionImpl::CollectionLoaded(BookmarkRootFolder* new_root) {
  root_folder_.reset(new_root);

  if (root_folder_)
    OnCollectionLoaded();
}

void BookmarkCollectionImpl::CollectionSaved() {
  is_saving_ = false;

  DCHECK(pending_save_.is_null());

  if (storage_dirty_) {
    storage_dirty_ = false;
    UpdatePersistentStorage();
  }
}

void BookmarkCollectionImpl::UpdatePersistentStorage() {
  if (!root_folder_ || data_store_->IsStopped())
    return;

  if (is_saving_) {
    storage_dirty_ = true;
    return;
  }

  if (pending_save_.is_null()) {
    pending_save_ = base::Bind(&BookmarkCollectionImpl::SaveToStorage,
                               weak_ptr_factory_.GetWeakPtr(),
                               false);
    ui_task_runner_->PostDelayedTask(
        FROM_HERE, pending_save_, base::TimeDelta::FromSeconds(3));
  }
}

void BookmarkCollectionImpl::UpdatePersistentStorageOnExit() {
  DCHECK(data_store_->IsStopped());
  if (!root_folder_ || !storage_dirty_)
    return;

  SaveToStorage(true);
}

void BookmarkCollectionImpl::SaveToStorage(bool force_save_on_exit) {
  pending_save_.Reset();
  is_saving_ = true;

  scoped_refptr<BookmarkStorage::BookmarkRawDataHolder> holder(
      new BookmarkStorage::BookmarkRawDataHolder());

  BookmarkUtils::ForEachBookmarkInFolder(
      root_folder_.get(), true, base::Bind(&PushBookmarkDataToStorage, holder));

  if (force_save_on_exit) {
    data_store_->SafeSaveBookmarkDataOnExit(holder);
  } else {
    data_store_->SaveBookmarkData(
        holder,
        base::Bind(&BookmarkCollectionImpl::CollectionSaved,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

BookmarkRootFolder::BookmarkRootFolder()
    : BookmarkFolder(base::string16(), std::string()) {
  // Empty ID for the root folder for two reasons:
  // 1. ID should not be regenerated every time for the root folder
  // 2. We want to support old bookmarks implementation where subfolders in the root folder had no
  // parents (and such bookmarks have empty parent id in the database)
  guid_ = "";
}

void BookmarkRootFolder::AddChildAt(BookmarkFolder* parent,
                                    Bookmark* child,
                                    int* index) {
  BookmarkFolder* real_parent = parent ? parent : this;

  if (*index == BookmarkCollection::kIndexPositionLast)
    *index = real_parent->GetChildCount();

  real_parent->AddChildAt(child, *index);
}

bool BookmarkRootFolder::IsRootFolder() const {
  return true;
}


void BookmarkRootFolder::Remove(Bookmark* bookmark) {
  bookmark->parent()->RemoveChild(bookmark);
}

void BookmarkRootFolder::Move(Bookmark* bookmark,
                              BookmarkFolder* move_to_folder,
                              int move_from_index,
                              int move_to_index) {
  BookmarkFolder* real_target = move_to_folder ? move_to_folder : this;

  if (move_to_index == BookmarkCollection::kIndexPositionLast)
    move_to_index = real_target->GetChildCount();

  bookmark->parent()->MoveChildToFolder(
      bookmark,
      real_target,
      move_from_index,
      move_to_index);
}

int BookmarkRootFolder::GetChildCount() const {
  return BookmarkFolder::GetChildCount();
}

Bookmark* BookmarkRootFolder::GetChildByIndex(int index) const {
  return BookmarkFolder::GetChildByIndex(index);
}

bool BookmarkRootFolder::RecreateBookmark(const std::string& guid,
                                          const base::string16& name,
                                          const std::string& url,
                                          int type,
                                          const std::string& parent_guid) {
  BookmarkFolder* parent = NULL;
  if (!parent_guid.empty()) {
    Bookmark* b = FindByGUID(parent_guid);
    if (!b || b->GetType() != Bookmark::BOOKMARK_FOLDER)
      return false;
    parent = static_cast<BookmarkFolder*>(b);
  }

  Bookmark* fav = NULL;

  switch (type) {
    case Bookmark::BOOKMARK_SITE:
      fav = new BookmarkSite(name, url, guid);
      break;

    case Bookmark::BOOKMARK_FOLDER:
      fav = new BookmarkFolder(name, guid);
      break;

    default:
      DLOG(ERROR) << "Unknown bookmark type";
      break;
  }

  DCHECK(fav);

  int index = BookmarkCollection::kIndexPositionLast;

  AddChildAt(parent, fav, &index);

  return true;
}

bool BookmarkCollectionImpl::BookmarkURLComparator::operator()(
    const Bookmark* n1,
    const Bookmark* n2) const {
  return n1->url() < n2->url();
}

void BookmarkCollectionImpl::AddToOrderedByURLSet(const Bookmark* bookmark) {
  if (!bookmark->url().empty()) {
    base::AutoLock lock(bookmarks_ordered_by_url_lock_);
    bookmarks_ordered_by_url_set_.insert(bookmark);
  }
}

void BookmarkCollectionImpl::RemoveFromOrderedByURLSet(
    const Bookmark* bookmark) {
  if (!bookmark->url().empty() && !bookmarks_ordered_by_url_set_.empty()) {
    base::AutoLock lock(bookmarks_ordered_by_url_lock_);
    BookmarksOrderedByURLSet::iterator i =
        bookmarks_ordered_by_url_set_.find(bookmark);
    if (i != bookmarks_ordered_by_url_set_.end()) {
      // i points to the first node with the URL, advance until we find the
      // node we're removing.
      while (*i != bookmark)
        ++i;

      bookmarks_ordered_by_url_set_.erase(i);
    }
  }
}

}  // namespace opera
