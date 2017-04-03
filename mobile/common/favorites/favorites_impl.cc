// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include <stdint.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/location.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_match.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/browser/opera_bookmark_utils.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/prefs/scoped_user_pref_update.h"

#include "mobile/common/favorites/device_folder_impl.h"
#include "mobile/common/favorites/favorite_suggestion_provider.h"
#include "mobile/common/favorites/favorites_impl.h"
#include "mobile/common/favorites/folder_impl.h"
#include "mobile/common/favorites/meta_info.h"
#include "mobile/common/favorites/node_favorite_impl.h"
#include "mobile/common/favorites/node_folder_impl.h"
#include "mobile/common/favorites/root_folder_impl.h"
#include "mobile/common/favorites/savedpage_impl.h"
#include "mobile/common/favorites/savedpages.h"
#include "mobile/common/favorites/special_folder_impl.h"
#include "mobile/common/favorites/title_match.h"

using bookmarks::BookmarkNode;

namespace {

const std::string kOldRootGUIDKey("old_root_guid");

// Note: These are home-brewn hard-coded mage-approved special GUIDs for
// identifying virtual folders.
const std::string kSavedPagesGUID("b0dd39f6-1ce5-457f-b37f-82e8e4cc75a6");
const std::string kBookmarksGUID("2962baa6-ada0-4b73-8ccd-1580854237f9");
const std::string kFeedsGUID("74276551-02f1-4aa0-a83d-1049a25f20c1");

const int32_t kBookmarksSpecialID = 1;
const int32_t kSavedPagesSpecialID = 2;
const int32_t kFeedsSpecialID = 3;

GURL AddMissingHTTPIfNeeded(const std::string& url) {
  GURL the_url = GURL(url);
  // It this is already a valid URL, just return it.
  if (the_url.is_valid())
    return the_url;
  // Otherwise try to add http://.
  return GURL("http://" + url);
}

int32_t FavoritePushId(mobile::Favorite* favorite) {
  if (favorite->IsFolder()) {
    return static_cast<mobile::Folder*>(favorite)->PartnerGroup();
  } else {
    return favorite->PartnerId();
  }
}

int64_t FavoriteNodeId(mobile::Favorite* favorite) {
  return favorite->data() ? favorite->data()->id() : 0;
}

void RecursiveRemove(bookmarks::BookmarkModel* model,
                     const BookmarkNode* parent,
                     int index,
                     const BookmarkNode* node) {
  if (node->is_folder()) {
    while (node->child_count() > 0) {
      int last = node->child_count() - 1;
      RecursiveRemove(model, node, last, node->GetChild(last));
    }
  }
  model->Remove(parent->GetChild(index));
}

}  // namespace

namespace mobile {

class PlaceholderFolder : public SpecialFolderImpl {
 public:
  PlaceholderFolder(FavoritesImpl* favorites,
                    int64_t parent,
                    int64_t id,
                    int32_t special_id,
                    const base::string16& title,
                    const std::string& guid)
      : SpecialFolderImpl(favorites, parent, id, special_id, title, guid) {}

  bool CanChangeParent() const override {
    return false;
  }

  bool CanChangeTitle() const override {
    return false;
  }

  bool CanTakeMoreChildren() const override {
    return false;
  }

 protected:
  bool Check() override {
    return false;
  }
};

class BookmarksFolder : public PlaceholderFolder {
 public:
  BookmarksFolder(FavoritesImpl* favorites,
                  int64_t parent,
                  int64_t id,
                  const base::string16& title)
      : PlaceholderFolder(favorites, parent, id, kBookmarksSpecialID, title,
                          kBookmarksGUID) {}
};

class FeedsFolder : public PlaceholderFolder {
 public:
  FeedsFolder(FavoritesImpl* favorites,
              int64_t parent,
              int64_t id,
              const base::string16& title)
      : PlaceholderFolder(favorites, parent, id, kFeedsSpecialID, title,
                          kFeedsGUID) {}
};

FavoritesImpl::FavoritesImpl()
    : devices_root_(NULL),
      local_root_(NULL),
      bookmarks_folder_(NULL),
      saved_pages_(NULL),
      backend_saved_pages_(NULL),
      feeds_(NULL),
      last_id_(0),
      icon_size_(0),
      thumb_size_(0),
      delegate_(NULL),
      bookmark_model_(NULL),
      loaded_(false),
      skip_thumbnail_search_(false),
      importing_from_model_(false),
      import_finished_(false),
      waiting_for_saved_pages_(false),
      inside_extensive_change_(false),
      after_extensive_change_posted_(false),
      update_partner_content_node_(NULL),
      bookmarks_folder_enabled_(0),
      feeds_enabled_(0),
      saved_pages_mode_(ENABLED),
      local_add_offset_(-1) {
  MetaInfo::Register();
  devices_root_ = new RootFolderImpl(this, GenerateId(), NULL);
  favorites_[devices_root_->id()] = devices_root_;
  local_root_ = new DeviceRootFolderImpl(this, devices_root_->id(),
      GenerateId(), base::UTF8ToUTF16("[root]"), NULL);
  favorites_[local_root_->id()] = local_root_;
  devices_root_->Add(local_root_);
  import_folder_ = local_root_->id();
}

FavoritesImpl::~FavoritesImpl() {
  if (delegate_) {
    delegate_->SetPartnerContentInterface(NULL);
  }
  if (bookmark_model_)
    bookmark_model_->RemoveObserver(this);
  for (auto i(favorites_.begin()); i != favorites_.end(); ++i) {
    delete i->second;
  }
}

bool FavoritesImpl::IsReady() {
  return prefs_ && !device_name_.empty() &&
      bookmark_model_ && bookmark_model_->loaded();
}

bool FavoritesImpl::IsLoaded() {
  return IsReady() && loaded_ && import_finished_ && !waiting_for_saved_pages_;
}

Folder* FavoritesImpl::devices_root() {
  return devices_root_;
}

Folder* FavoritesImpl::local_root() {
  return local_root_;
}

Folder* FavoritesImpl::bookmarks_folder() {
  return bookmarks_folder_;
}

Folder* FavoritesImpl::saved_pages() {
  return backend_saved_pages_;
}

Folder* FavoritesImpl::feeds() {
  return feeds_;
}

void FavoritesImpl::SetBookmarksFolderTitle(const base::string16& title) {
  bookmarks_folder_title_ = title;
  if (bookmarks_folder_)
    bookmarks_folder_->SetTitle(title);
}

void FavoritesImpl::SetSavedPagesTitle(const base::string16& title) {
  saved_pages_title_ = title;
  if (backend_saved_pages_)
    backend_saved_pages_->SetTitle(title);
}

void FavoritesImpl::SetFeedsTitle(const base::string16& title) {
  feeds_title_ = title;
  if (feeds_)
    feeds_->SetTitle(title);
}

void FavoritesImpl::SetRequestGraphicsSizes(int icon_size, int thumb_size) {
  icon_size_ = icon_size;
  thumb_size_ = thumb_size;
  if (delegate_) {
    delegate_->SetRequestGraphicsSizes(icon_size, thumb_size);
  }
}

void FavoritesImpl::SetDelegate(FavoritesDelegate* delegate) {
  if (delegate == delegate_)
    return;
  if (delegate_) {
    delegate_->SetPartnerContentInterface(NULL);
  }
  delegate_ = delegate;
  if (!delegate)
    return;
  const char* path_suffix = delegate_->GetPathSuffix();
  path_suffix_ = path_suffix ? path_suffix : "";
  if (thumb_size_ > 0) {
    delegate_->SetRequestGraphicsSizes(icon_size_, thumb_size_);
  }
  if (loaded_ &&                // Bookmark model loaded.
      import_finished_) {       // Favorites imported.
    delegate_->SetPartnerContentInterface(this);
  }
  SendDelayedThumbnailRequests();
}

void FavoritesImpl::SendDelayedThumbnailRequests() {
  if (delegate_ && !path_.empty()) {
    while (!delayed_thumbnail_requests_.empty()) {
      Favorite* favorite = *delayed_thumbnail_requests_.begin();
      delayed_thumbnail_requests_.erase(delayed_thumbnail_requests_.begin());
      RequestThumbnail(favorite);
    }
  }
}

void FavoritesImpl::SetModel(
    bookmarks::BookmarkModel* bookmark_model,
    const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner,
    const scoped_refptr<base::SingleThreadTaskRunner>& file_task_runner) {
  if (bookmark_model_) {
    bookmark_model_->RemoveObserver(this);
  }

  bookmark_model_ = bookmark_model;
  ui_task_runner_ = ui_task_runner;
  file_task_runner_ = file_task_runner;

  CheckPreferences(false);

  bookmark_model_->AddObserver(this);

  if (bookmark_model_->loaded()) {
    BookmarkModelLoaded(bookmark_model_, false);
  }
}

void FavoritesImpl::SetDeviceName(const std::string& name) {
  device_name_ = name;

  if (bookmark_model_ && bookmark_model_->loaded()) {
    BookmarkModelLoaded(bookmark_model_, false);
  }
}

void FavoritesImpl::SetSavedPagesEnabled(SavedPagesMode mode) {
  DCHECK(!loaded_);
  saved_pages_mode_ = mode;
}

void FavoritesImpl::SetBaseDirectory(const std::string& path) {
  path_ = path;
  if (path_.empty() || path_[path_.size() - 1] != '/') {
    path_ += "/";
  }
  CheckPreferences(true);
  SendDelayedThumbnailRequests();
}

void FavoritesImpl::SetSavedPageDirectory(const std::string& path) {
  savedpage_path_ = path;
  if (savedpage_path_.empty() ||
      savedpage_path_[savedpage_path_.size() - 1] != '/') {
    savedpage_path_ += "/";
  }
  if (bookmark_model_ && bookmark_model_->loaded()) {
    ReadSavedPages();
  }
}

Folder* FavoritesImpl::CreateFolder(int pos, const base::string16& title) {
  BeginLocalAdd();
  const BookmarkNode* data = bookmark_model_->AddFolder(
      local_root_->data(),
      IndexToModel(local_root_, pos),
      title);
  EndLocalAdd();
  auto folder = static_cast<Folder*>(model_favorites_[data->id()]);
  if (inside_extensive_change_)
    DisableCheck(folder);
  return folder;
}

Folder* FavoritesImpl::CreateFolder(int pos,
                                    const base::string16& title,
                                    int32_t pushed_partner_group_id) {
  DCHECK_NE(pushed_partner_group_id, 0);
  return CreateFolder(pos, title, "", pushed_partner_group_id);
}

Folder* FavoritesImpl::CreateFolder(int pos,
                                    const base::string16& title,
                                    const std::string& guid,
                                    int32_t pushed_partner_group_id) {
  BookmarkNode::MetaInfoMap meta_info;
  BookmarkNode::MetaInfoMap* p_meta_info = NULL;
  if (!guid.empty()) {
    MetaInfo::SetGUID(meta_info, guid);
    p_meta_info = &meta_info;
  }
  if (pushed_partner_group_id) {
    MetaInfo::SetPushedPartnerGroupId(meta_info, pushed_partner_group_id);
    p_meta_info = &meta_info;
  }
  BeginLocalAdd();
  const BookmarkNode* data = bookmark_model_->AddFolderWithMetaInfo(
      local_root_->data(),
      IndexToModel(local_root_, pos),
      title,
      p_meta_info);
  EndLocalAdd();
  auto folder = static_cast<Folder*>(model_favorites_[data->id()]);
  if (inside_extensive_change_)
    DisableCheck(folder);
  return folder;
}

Favorite* FavoritesImpl::CreateFavorite(Folder* parent,
                                        int pos,
                                        const base::string16& title,
                                        const GURL& url) {
  Favorite* favorite = CreateFavorite(parent, pos, title, url, "", false);
  RequestThumbnail(favorite);
  return favorite;
}

Favorite* FavoritesImpl::CreateFavorite(Folder* parent,
                                        int pos,
                                        const base::string16& title,
                                        const GURL& url,
                                        const std::string& guid,
                                        bool look_for_thumbnail) {
  DCHECK(!skip_thumbnail_search_);
  skip_thumbnail_search_ = !look_for_thumbnail;
  BookmarkNode::MetaInfoMap meta_info;
  const BookmarkNode::MetaInfoMap* p_meta_info;
  if (!guid.empty()) {
    MetaInfo::SetGUID(meta_info, guid);
    p_meta_info = &meta_info;
  } else {
    p_meta_info = NULL;
  }
  BeginLocalAdd();
  const BookmarkNode* data = bookmark_model_->AddURLWithCreationTimeAndMetaInfo(
      parent->data(),
      IndexToModel(parent, pos),
      title,
      url,
      base::Time::Now(),
      p_meta_info);
  EndLocalAdd();
  return model_favorites_[data->id()];
}

SavedPage* FavoritesImpl::CreateSavedPage(const base::string16& title,
                                          const GURL& url,
                                          const std::string& file) {
  Folder* folder = CreateSavedPagesFolder();
  const std::string guid =
      "savedpage:" + base::FilePath(file).BaseName().RemoveExtension().value();
  SavedPage* savedpage = new SavedPageImpl(
      this, folder->id(), GenerateId(), title, url, guid, file, "");
  favorites_[savedpage->id()] = savedpage;
  folder->Add(savedpage);
  RequestThumbnail(savedpage);
  return savedpage;
}

Favorite* FavoritesImpl::CreatePartnerContent(
    Folder* parent,
    int pos,
    const base::string16& title,
    const GURL& url,
    const std::string& guid,
    int32_t pushed_partner_activation_count,
    int32_t pushed_partner_channel,
    int32_t pushed_partner_id,
    int32_t pushed_partner_checksum,
    bool look_for_thumbnail) {
  DCHECK_NE(pushed_partner_id, 0);
  DCHECK(!skip_thumbnail_search_);
  skip_thumbnail_search_ = !look_for_thumbnail;
  BookmarkNode::MetaInfoMap meta_info;
  MetaInfo::SetPushedPartnerActivationCount(meta_info,
                                            pushed_partner_activation_count);
  MetaInfo::SetPushedPartnerChannel(meta_info, pushed_partner_channel);
  MetaInfo::SetPushedPartnerId(meta_info, pushed_partner_id);
  MetaInfo::SetPushedPartnerChecksum(meta_info, pushed_partner_checksum);
  if (!guid.empty()) {
    MetaInfo::SetGUID(meta_info, guid);
  }
  BeginLocalAdd();
  const BookmarkNode* data = bookmark_model_->AddURLWithCreationTimeAndMetaInfo(
      parent->data(),
      IndexToModel(parent, pos),
      title,
      url,
      base::Time::Now(),
      &meta_info);
  EndLocalAdd();
  return model_favorites_[data->id()];
}

void FavoritesImpl::Remove(int64_t id) {
  Favorite* favorite = GetFavorite(id);
  if (favorite) {
    if (favorite == bookmarks_folder_) {
      return;
    }
    Folder* parent = favorite->GetParent();
    favorite->Remove();
    parent->Check();
  }
}

Favorite* FavoritesImpl::GetFavorite(int64_t id) {
  favorites_t::iterator i(favorites_.find(id));
  return i != favorites_.end() ? i->second : NULL;
}

void FavoritesImpl::AddObserver(FavoritesObserver* observer) {
  observers_.AddObserver(observer);
}

void FavoritesImpl::RemoveObserver(FavoritesObserver* observer) {
  observers_.RemoveObserver(observer);
}

void FavoritesImpl::SignalReady() {
  DCHECK(IsReady());
  FOR_EACH_OBSERVER(FavoritesObserver, observers_, OnReady());
}

void FavoritesImpl::SignalLoaded() {
  DCHECK(IsLoaded());
  FOR_EACH_OBSERVER(FavoritesObserver, observers_, OnLoaded());
}

void FavoritesImpl::SignalAdded(int64_t id, int64_t parent, int pos) {
  if (parent == local_root_->id()) {
    UpdateSpecialIndexes();
  }

  FOR_EACH_OBSERVER(FavoritesObserver, observers_, OnAdded(id, parent, pos));
}

void FavoritesImpl::SignalMoved(int64_t id,
                                int64_t old_parent,
                                int old_pos,
                                int64_t new_parent,
                                int new_pos) {
  Favorite* favorite = GetFavorite(id);

  if (new_parent == local_root_->id()) {
    UpdateSpecialIndexes();
  }

  const BookmarkNode* node = favorite->data();
  if (node) {
    Folder* parent = static_cast<Folder*>(GetFavorite(new_parent));
    if (old_parent == new_parent) {
      int model_old_pos = IndexToModel(parent, old_pos);
      BeginLocalAdd();
      int model_new_pos = IndexToModel(parent, new_pos);
      bookmark_model_->Move(node, parent->data(),
                            model_new_pos >= model_old_pos ?
                            model_new_pos + 1 : model_new_pos);
      EndLocalAdd();
    } else {
      BeginLocalAdd();
      int model_new_pos = IndexToModel(parent, new_pos);
      bookmark_model_->Move(node, parent->data(), model_new_pos);
      EndLocalAdd();
    }
  }

  // Make sure folders which used to contain pushed content isn't tagged as a
  // pushed folder after moving out the last pushed element
  if (!favorite->IsFolder() && old_parent != new_parent) {
    UpdatePushedGroupId(old_parent);
  }

  FOR_EACH_OBSERVER(FavoritesObserver,
                    observers_,
                    OnMoved(id, old_parent, old_pos, new_parent, new_pos));
}

void FavoritesImpl::SignalRemoved(int64_t id,
                                  int64_t old_parent,
                                  int old_pos,
                                  bool recursive) {
  if (old_parent == local_root_->id() && local_root_->data()) {
    UpdateSpecialIndexes();
  }

  FOR_EACH_OBSERVER(FavoritesObserver,
                    observers_,
                    OnRemoved(id, old_parent, old_pos, recursive));
}

void FavoritesImpl::SignalChanged(int64_t id,
                                  int64_t parent,
                                  unsigned int changed) {
  FOR_EACH_OBSERVER(
      FavoritesObserver, observers_, OnChanged(id, parent, changed));
}

int64_t FavoritesImpl::GenerateId() {
  favorites_t::iterator i(favorites_.find(last_id_));
  while (i != favorites_.end()) {
    i = favorites_.find(++last_id_);
  }
  return last_id_++;
}

static void DeleteFileTask(const base::FilePath& file) {
  base::DeleteFile(file, false);
}

void FavoritesImpl::DeleteSavedPageFile(const std::string& file) {
  file_task_runner_->PostTask(
      FROM_HERE, base::Bind(&DeleteFileTask, base::FilePath(file)));
}

void FavoritesImpl::RemoveNodeFavorite(Favorite* favorite) {
  const BookmarkNode* node = favorite->data();
  RecursiveRemove(bookmark_model_, node->parent(),
                  node->parent()->GetIndexOf(node), node);
}

void FavoritesImpl::RemoveNonNodeFavorite(Favorite* favorite) {
  if (favorite->GetSpecialId() != 0) {
    SetSpecialIndex(favorite->GetSpecialId(), -1);
    if (favorite == bookmarks_folder_) {
      bookmarks_folder_ = NULL;
    } else if (favorite == backend_saved_pages_) {
      saved_pages_ = NULL;
      backend_saved_pages_ = NULL;
    } else if (favorite == feeds_) {
      feeds_ = NULL;
    }
  }

  // Clean up parent mapping
  int64_t parent = favorite->parent();
  Folder* parentFolder = favorite->GetParent();
  int position;
  if (parentFolder != favorite) {
    position = favorite->GetParent()->IndexOf(favorite);
    favorite->GetParent()->ForgetAbout(favorite);
  } else {
    position = 0;
  }

  // Signal to UI about the changes
  SignalRemoved(favorite->id(), parent, position, false);

  // Clean up FavoritesImpl mapping
  favorites_t::iterator j(favorites_.find(favorite->id()));
  if (j != favorites_.end())
    favorites_.erase(j);

  delete favorite;
}

std::string FavoritesImpl::GenerateThumbnailName(Favorite* favorite) {
  std::string path(favorite->thumbnail_path());
  if (path.empty() || path.size() < path_.size() ||
      path.compare(0, path_.size(), path_) != 0) {
    path = GetFavoriteBaseName(favorite->guid(), FavoriteNodeId(favorite));
  } else {
    path = path.substr(path_.size());
  }
  return path;
}

void FavoritesImpl::RequestThumbnail(Favorite* favorite) {
  Favorite::ThumbnailMode mode = favorite->GetThumbnailMode();
  if (mode == Favorite::ThumbnailMode::NONE) return;
  if (delegate_ && !path_.empty()) {
    const std::string name(GenerateThumbnailName(favorite));
    const std::string url(favorite->url().spec());
    if (mode == Favorite::ThumbnailMode::DOWNLOAD) {
      delegate_->RequestThumbnail(
          favorite->id(), url.c_str(), name.c_str(),
          favorite->CanChangeTitle() && favorite->title().empty(), this);
    } else {
      DCHECK_EQ(Favorite::ThumbnailMode::FALLBACK, mode);
      // Generate empty file in path
      file_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&FavoritesImpl::StoreThumbnail,
                     base::Unretained(this),
                     favorite->id(),
                     path_ + name,
                     std::vector<char>()));
    }
  } else {
    delayed_thumbnail_requests_.insert(favorite);
  }
}

void FavoritesImpl::CancelThumbnailRequest(Favorite* favorite) {
  if (delegate_) {
    delegate_->CancelThumbnailRequest(favorite->id());
  } else {
    delayed_thumbnail_requests_.erase(favorite);
  }
}

void FavoritesImpl::Finished(int64_t id, const char* title) {
  Favorite* favorite = GetFavorite(id);
  if (favorite) {
    // TODO(the_jk): Don't recalculate the name
    favorite->SetThumbnailPath(path_ + GenerateThumbnailName(favorite));
    if (favorite->CanChangeTitle() && favorite->title().empty())
      favorite->SetTitle(base::UTF8ToUTF16(title));
  }
}

void FavoritesImpl::Error(int64_t id, bool temporary) {
  if (!temporary) {
    auto favorite = GetFavorite(id);
    if (favorite) {
      // Generate empty file in path
      file_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&FavoritesImpl::StoreThumbnail,
                     base::Unretained(this),
                     favorite->id(),
                     path_ + GenerateThumbnailName(favorite),
                     std::vector<char>()));
    }
  }
}

void FavoritesImpl::ReportPartnerContentEntryActivated(Favorite* favorite) {
  delegate_->OnPartnerContentActivated();
}

void FavoritesImpl::CheckBookmarksFolder() {
  if (!bookmark_model_ || !bookmark_model_->loaded())
    return;

  if (bookmarks_folder_enabled_ > 0 && !bookmarks_folder_) {
    int index = GetSpecialIndex(kBookmarksSpecialID);
    if (index < 0) {
      // Default location is after all the partner content
      index = -1;
      for (int i = 0; i < local_root_->Size(); i++) {
        if (local_root_->Child(i)->IsPartnerContent()) {
          index = i;
        }
      }
      index++;
    }
    bookmarks_folder_ = new BookmarksFolder(this, local_root_->id(),
        GenerateId(), bookmarks_folder_title_);
    favorites_[bookmarks_folder_->id()] = bookmarks_folder_;
    if (inside_extensive_change_)
      DisableCheck(bookmarks_folder_);
    local_root_->Add(index, bookmarks_folder_);
  } else if (bookmarks_folder_enabled_ < 0 && bookmarks_folder_) {
    bookmarks_folder_->Remove();
    DCHECK(!bookmarks_folder_);
  }
}

void FavoritesImpl::SetBookmarksEnabled(bool enabled) {
  bookmarks_folder_enabled_ = enabled ? 1 : -1;
  if (loaded_) {
    CheckBookmarksFolder();
  }
}

void FavoritesImpl::CheckFeeds() {
  if (feeds_enabled_ > 0 && !feeds_) {
    int index = GetSpecialIndex(kFeedsSpecialID);
    if (index < 0) {
      if (saved_pages_) {
        index = local_root_->IndexOf(saved_pages_) + 1;
      } else if (bookmarks_folder_) {
        index = local_root_->IndexOf(bookmarks_folder_) + 1;
      } else {
        index = local_root_->Size();
      }
    }
    feeds_ =
        new FeedsFolder(this, local_root_->id(), GenerateId(), feeds_title_);
    favorites_[feeds_->id()] = feeds_;
    if (inside_extensive_change_)
      DisableCheck(feeds_);
    local_root_->Add(index, feeds_);
  } else if (feeds_enabled_ < 0 && feeds_) {
    feeds_->Remove();
    DCHECK(!feeds_);
  }
}

void FavoritesImpl::SetFeedsEnabled(bool enabled) {
  feeds_enabled_ = enabled ? 1 : -1;
  if (loaded_) {
    CheckFeeds();
  }
}

void FavoritesImpl::OpenFolder(const char* title) {
  import_folder_ =
      CreateFolder(local_root_->Size(), base::UTF8ToUTF16(title))->id();
}

void FavoritesImpl::OpenFolder(const char* title, int32_t pushed_group_id) {
  import_folder_ = CreateFolder(local_root_->Size(),
                                base::UTF8ToUTF16(title),
                                pushed_group_id)->id();
}

void FavoritesImpl::CloseFolder() {
  import_folder_ = local_root_->id();
}

void FavoritesImpl::ImportFavorite(const char* title,
                                   const char* url,
                                   const char* thumbnail_path) {
  Folder* folder = static_cast<Folder*>(GetFavorite(import_folder_));
  if (!folder) {
    folder = local_root_;
  }

  GURL gurl = AddMissingHTTPIfNeeded(url);
  if (gurl.is_valid()) {
    if (thumbnail_path != NULL) {
      // Call non thumb-requesting create method since we will try to migrate
      // the Bream thumbnail. Only if that fails will we request a new thumb.
      Favorite* favorite = CreateFavorite(folder,
                                          folder->Size(),
                                          base::UTF8ToUTF16(title),
                                          gurl,
                                          "",
                                          false);

      file_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&FavoritesImpl::MoveBreamFavoriteThumbnail,
                     base::Unretained(this),
                     path_ + thumbnail_path,
                     favorite->id(),
                     favorite->guid(),
                     FavoriteNodeId(favorite)));
    } else {
      CreateFavorite(folder,
          folder->Size(),
          base::UTF8ToUTF16(title),
          gurl);
    }
  } else {
    LOG(WARNING) << "Tried to import favorite with invalid URL " << std::string(url);
  }
}

void FavoritesImpl::ImportPartnerContent(
    const char* title,
    const char* url,
    const char* thumbnail_path,
    int32_t pushed_partner_activation_count,
    int32_t pushed_partner_channel,
    int32_t pushed_partner_id,
    int32_t pushed_partner_checksum) {
  Folder* folder = static_cast<Folder*>(GetFavorite(import_folder_));
  if (!folder) {
    folder = local_root_;
  }

  Favorite* favorite = CreatePartnerContent(folder,
                                            folder->Size(),
                                            base::UTF8ToUTF16(title),
                                            AddMissingHTTPIfNeeded(url),
                                            "",
                                            pushed_partner_activation_count,
                                            pushed_partner_channel,
                                            pushed_partner_id,
                                            pushed_partner_checksum,
                                            false);

  file_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&FavoritesImpl::MoveBreamFavoriteThumbnail,
                 base::Unretained(this),
                 path_ + thumbnail_path,
                 favorite->id(),
                 favorite->guid(),
                 FavoriteNodeId(favorite)));
}

void FavoritesImpl::ImportSavedPage(const char* title,
                                    const char* url,
                                    const char* file,
                                    const char* thumbnail_path) {
  // Try to avoid the crash in ANDUI-3100 by removing any possibility of null
  // initializing the std::strings.
  std::string title_str(title ? title : "");
  std::string file_str(file ? file : "");
  GURL url_url(AddMissingHTTPIfNeeded(url ? url : ""));
  std::string thumbnail_path_str(thumbnail_path ? thumbnail_path : "");

  ImportSavedPage(title_str, file_str, url_url, thumbnail_path_str);
}

void FavoritesImpl::ImportSavedPage(const std::string& title,
                                    const std::string& file,
                                    const GURL& url,
                                    const std::string& thumbnail_path) {
  SavedPage* saved_page =
      CreateSavedPage(base::UTF8ToUTF16(title), url, file);
  file_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&FavoritesImpl::MoveBreamFavoriteThumbnail,
                 base::Unretained(this),
                 path_ + thumbnail_path,
                 saved_page->id(),
                 saved_page->guid(),
                 0));
}

void FavoritesImpl::Flush() {
  if (backend_saved_pages_) {
    backend_saved_pages_->DoScheduledWrite();
  }
}

void FavoritesImpl::ImportBookmarkModel() {
  DCHECK(!importing_from_model_);
  importing_from_model_ = true;
  ExtensiveBookmarkChangesBeginning(bookmark_model_);
  const BookmarkNode* const src = devices_root_->data();
  for (int i = 0; i < src->child_count(); i++) {
    const BookmarkNode* const child = src->GetChild(i);
    if (child->type() == BookmarkNode::FOLDER) {
      Folder* folder;
      if (child != local_root_->data()) {
        BookmarkNodeAdded(bookmark_model_, src, i);
        auto it = model_favorites_.find(child->id());
        if (it == model_favorites_.end()) continue;
        folder = static_cast<Folder*>(it->second);
      } else {
        folder = local_root_;
      }
      ImportFolder(folder, child);
    }
  }
  ExtensiveBookmarkChangesEnded(bookmark_model_);
  // Call AfterExtensiveBookmarkChangesEnded directly here instead of waiting
  // for the task to run on the UI task runner
  AfterExtensiveBookmarkChangesEnded();
  DCHECK(importing_from_model_);
  importing_from_model_ = false;
}

void FavoritesImpl::ImportFolder(Folder* dst, const BookmarkNode* src) {
  for (int i = 0; i < src->child_count(); i++) {
    const BookmarkNode* child = src->GetChild(i);
    switch (child->type()) {
      case BookmarkNode::URL:
        BookmarkNodeAdded(bookmark_model_, src, i);
        break;
      case BookmarkNode::FOLDER: {
        BookmarkNodeAdded(bookmark_model_, src, i);
        auto it = model_favorites_.find(child->id());
        if (it != model_favorites_.end()) {
          ImportFolder(static_cast<Folder*>(it->second), child);
        }
        break;
      }
      default:
        break;
    }
  }
}

void FavoritesImpl::MoveBreamFavoriteThumbnail(const std::string& current_path,
                                               int64_t id,
                                               const std::string& guid,
                                               int64_t node_id) {
  base::FilePath from(current_path);
  base::FilePath to(path_ + GetFavoriteBaseName(guid, node_id));
  if (base::Move(from, to)) {
    ui_task_runner_->PostTask(FROM_HERE,
                              base::Bind(&FavoritesImpl::OnThumbnailFound,
                                         base::Unretained(this),
                                         id,
                                         to.value()));
  } else {
    ui_task_runner_->PostTask(FROM_HERE,
                              base::Bind(&FavoritesImpl::OnThumbnailFound,
                                         base::Unretained(this),
                                         id,
                                         ""));
  }
}

SavedPagesFolder* FavoritesImpl::CreateSavedPagesFolder() {
  if (!backend_saved_pages_) {
    std::unique_ptr<SavedPagesWriter> writer(
        new SavedPagesWriter(file_task_runner_,
                             base::FilePath(path_),
                             base::FilePath(savedpage_path_)));
    switch (saved_pages_mode_) {
      case ENABLED:
      case ALWAYS: {
        int index = GetSpecialIndex(kSavedPagesSpecialID);
        if (index < 0) {
          // Default location is to put it right after the bookmarks folder
          if (bookmarks_folder_) {
            index = local_root_->IndexOf(bookmarks_folder_) + 1;
          } else {
            index = local_root_->Size();
          }
        }
        backend_saved_pages_ = new SavedPagesFolder(
            this, local_root_->id(), GenerateId(), kSavedPagesSpecialID,
            saved_pages_title_, kSavedPagesGUID, std::move(writer),
            saved_pages_mode_ == ALWAYS);
        favorites_[backend_saved_pages_->id()] = backend_saved_pages_;
        if (inside_extensive_change_)
          DisableCheck(backend_saved_pages_);
        saved_pages_ = backend_saved_pages_;
        local_root_->Add(index, saved_pages_);
        break;
      }
      case DISABLED: {
        int64_t id = GenerateId();
        backend_saved_pages_ = new SavedPagesFolder(
            this, id, id, kSavedPagesSpecialID, saved_pages_title_,
            kSavedPagesGUID, std::move(writer), false);
        favorites_[backend_saved_pages_->id()] = backend_saved_pages_;
        if (inside_extensive_change_)
          DisableCheck(backend_saved_pages_);
        // Generate a "fake" added signal
        SignalAdded(backend_saved_pages_->id(), backend_saved_pages_->id(), 0);
        break;
      }
    }
  }
  return backend_saved_pages_;
}

SavedPage* FavoritesImpl::CreateSavedPage(const base::string16& title,
                                          const GURL& url,
                                          const std::string& guid,
                                          const std::string& file) {
  const std::string guid2 = "savedpage:" + guid;
  std::string thumbnail_path;
  if (!path_.empty()) {
    thumbnail_path = path_ + GetFavoriteBaseName(guid2, 0);
  }
  SavedPage* savedpage = new SavedPageImpl(
      this, backend_saved_pages_->id(), GenerateId(), title, url, guid2, file,
      thumbnail_path);
  favorites_[savedpage->id()] = savedpage;
  if (!thumbnail_path.empty()) {
    file_task_runner_->PostTask(FROM_HERE,
                                base::Bind(&FavoritesImpl::LookForThumbnail,
                                           base::Unretained(this),
                                           savedpage->id(),
                                           thumbnail_path,
                                           true));
  }
  return savedpage;
}

void FavoritesImpl::DoneReadingSavedPages() {
  saved_pages_reader_.reset(NULL);
  if (backend_saved_pages_) {
    // If we have for some reason become out of sync (Saved pages folder exists
    // but no saved pages) then this will remove the saves pages folder
    backend_saved_pages_->Check();
  }

  if (waiting_for_saved_pages_) {
    waiting_for_saved_pages_ = false;
    SignalLoaded();
  }
}

void FavoritesImpl::ReadSavedPages() {
  if (savedpage_path_.empty())
    return;

  // No need to read saved pages when import step has been run.
  // TODO(marejde): Come up with a less error-prone way to check this.
  if (backend_saved_pages_ && backend_saved_pages_->Size() > 0)
    return;

  saved_pages_reader_.reset(
      new SavedPagesReader(*this,
                           ui_task_runner_,
                           file_task_runner_,
                           base::FilePath(path_),
                           base::FilePath(savedpage_path_)));
  saved_pages_reader_->ReadSavedPages();
}

void FavoritesImpl::SetupRoot() {
  const BookmarkNode* node = NULL;
  static_cast<RootFolderImpl*>(devices_root_)->SetData(
      opera::OperaBookmarkUtils::GetSpeedDialNode(bookmark_model_));
  model_favorites_[devices_root_->data()->id()] = devices_root_;

  std::string root_guid =
      prefs_->GetString(opera::kBookmarkSpeedDialRootFolderGUIDKey);
  if (!root_guid.empty()) {
    for (int child = 0; child < devices_root_->data()->child_count(); child++) {
      std::string guid;
      node = devices_root_->data()->GetChild(child);
      node->GetMetaInfo(opera::kBookmarkSpeedDialRootFolderGUIDKey, &guid);
      if (guid == root_guid) {
        break;
      }
    }
  }
  if (!node) {
    if (root_guid.empty()) {
      root_guid = base::GenerateGUID();
      prefs_->SetString(opera::kBookmarkSpeedDialRootFolderGUIDKey, root_guid);
      prefs_->CommitPendingWrite();
    }
    BookmarkNode::MetaInfoMap map;
    map.insert(std::make_pair(opera::kBookmarkSpeedDialRootFolderGUIDKey,
                              root_guid));
    node = bookmark_model_->AddFolderWithMetaInfo(
        devices_root_->data(), 0, base::UTF8ToUTF16(device_name_), &map);
  }
  static_cast<RootFolderImpl*>(local_root_)->SetData(node);
  model_favorites_[node->id()] = local_root_;
}


void FavoritesImpl::BookmarkModelLoaded(bookmarks::BookmarkModel* model,
                                        bool ids_reassigned) {
  DCHECK_EQ(model, bookmark_model_);
  if (!prefs_ || device_name_.empty()) return;

  SetupRoot();

  SignalReady();
  ImportBookmarkModel();
  loaded_ = true;

  if (import_finished_ || !collection_reader_ ||
      collection_reader_->IsFinished()) {
    import_finished_ = true;
    FinishLoaded();
  }
}

void FavoritesImpl::FinishLoaded() {
  if (delegate_) {
    delegate_->SetPartnerContentInterface(this);
  }
  CheckBookmarksFolder();
  switch (saved_pages_mode_) {
  case ALWAYS:
    CreateSavedPagesFolder();
    break;
  case ENABLED:
  case DISABLED:
    break;
  }

  waiting_for_saved_pages_ = true;
  ReadSavedPages();
  CheckFeeds();
}

void FavoritesImpl::BookmarkModelBeingDeleted(
    bookmarks::BookmarkModel* model) {
  // TODO(the_jk): This is rather incomplete as we have no way of signaling
  // the UI that this happened. However, at the time of writing it will never
  // happen so ...
  DCHECK_EQ(model, bookmark_model_);
  bookmark_model_->RemoveObserver(this);
  bookmark_model_ = NULL;
  loaded_ = false;
}

void FavoritesImpl::LookForThumbnail(int64_t id, const std::string& path,
                                     bool ignore_if_exists) {
  DCHECK(!path.empty());
  if (!base::PathExists(base::FilePath(path))) {
    ui_task_runner_->PostTask(FROM_HERE,
                              base::Bind(&FavoritesImpl::OnThumbnailFound,
                                         base::Unretained(this),
                                         id,
                                         ""));
  } else if (!ignore_if_exists) {
    ui_task_runner_->PostTask(FROM_HERE,
                              base::Bind(&FavoritesImpl::OnThumbnailFound,
                                         base::Unretained(this),
                                         id,
                                         path));
  }
}

void FavoritesImpl::OnThumbnailFound(int64_t id, const std::string& path) {
  Favorite* favorite = GetFavorite(id);
  if (!favorite)
    return;
  favorite->SetThumbnailPath(path);
  if (path.empty()) {
    RequestThumbnail(favorite);
  }
}

bool FavoritesImpl::NodeIsPartOfFavorites(const BookmarkNode* node) {
  return node->HasAncestor(devices_root_->data());
}

void FavoritesImpl::SafeRemove(const BookmarkNode* parent, int index,
                               const BookmarkNode* node) {
  DCHECK(parent && node);
  if (inside_extensive_change_) {
    delayed_removal_.insert(node->id());
  } else {
    bookmark_model_->Remove(parent->GetChild(index));
  }
}

void FavoritesImpl::BookmarkNodeAdded(bookmarks::BookmarkModel* model,
                                      const BookmarkNode* node_parent,
                                      int index) {
  DCHECK_EQ(model, bookmark_model_);
  if (!local_root_->data()) return;
  if (!NodeIsPartOfFavorites(node_parent)) return;
  if (node_parent == devices_root_->data()) {
    std::string old_root_guid = prefs_->GetString(kOldRootGUIDKey);
    if (!old_root_guid.empty()) {
      const BookmarkNode* node = node_parent->GetChild(index);
      std::string guid;
      node->GetMetaInfo(opera::kBookmarkSpeedDialRootFolderGUIDKey, &guid);
      if (old_root_guid == guid) {
        SafeRemove(node_parent, index, node);
        prefs_->ClearPref(kOldRootGUIDKey);
        return;
      }
    }
  }
  const BookmarkNode* node = node_parent->GetChild(index);
  switch (node->type()) {
    case BookmarkNode::URL:
    case BookmarkNode::FOLDER:
      break;
    default:
      return;
  }

  // Already in the tree
  if (model_favorites_.find(node->id()) != model_favorites_.end())
    return;

  favorites_t::iterator i(model_favorites_.find(node_parent->id()));
  if (i == model_favorites_.end()) {
    // If a node is added to a parent we don't know about it's probably the old
    // root ignored above. Anyway, not much we can do here
    DLOG(INFO) << "Nonexistent parent: " << node_parent->id();
    return;
  }
  Folder* parent = static_cast<Folder*>(i->second);
  if (parent->parent() != devices_root_->id()) {
    DCHECK(!node->is_folder());
    if (node->is_folder()) return;
  } else if (parent == devices_root_) {
    DCHECK(node->is_folder());
    if (!node->is_folder()) return;
  }

  Favorite* item = NULL;
  std::string thumbnail_path;
  if (node->HasAncestor(local_root_->data())) {
    if (skip_thumbnail_search_) {
      skip_thumbnail_search_ = false;
    } else if (!path_.empty()) {
      thumbnail_path = path_ + GetFavoriteBaseName(MetaInfo::GetGUID(node),
                                                   node->id());
    }
  }
  if (!node->is_folder()) {
    // When importing from the model, assume the thumbnails will be there
    // and correct if it was the wrong assumption
    item = new NodeFavoriteImpl(this,
                                parent->id(),
                                GenerateId(),
                                node,
                                importing_from_model_ ?
                                thumbnail_path : "");
  } else {
    if (parent == local_root_) {
      const std::string guid(MetaInfo::GetGUID(node));
      if (guid == kBookmarksGUID || guid == kSavedPagesGUID ||
          guid == kFeedsGUID) {
        int special_index = index;
        if (bookmarks_folder_) special_index++;
        if (saved_pages_) special_index++;
        if (feeds_) special_index++;

        if (!bookmarks_folder_ && guid == kBookmarksGUID &&
            GetSpecialIndex(kBookmarksSpecialID) < 0) {
          SetSpecialIndex(kBookmarksSpecialID, special_index);
          CheckBookmarksFolder();
        } else if (!saved_pages_ && guid == kSavedPagesGUID &&
                   GetSpecialIndex(kSavedPagesSpecialID) < 0) {
          SetSpecialIndex(kSavedPagesSpecialID, special_index);
          CreateSavedPagesFolder();
        } else if (!feeds_ && guid == kFeedsGUID &&
                   GetSpecialIndex(kFeedsSpecialID) < 0) {
          SetSpecialIndex(kFeedsSpecialID, special_index);
          CheckFeeds();
        }
        SafeRemove(node_parent, index, node);
        return;
      }
    }
    if (parent != devices_root_) {
      item = new NodeFolderImpl(this, parent->id(), GenerateId(), node,
                                importing_from_model_ ? thumbnail_path : "");
    } else {
      item = new DeviceFolderImpl(this, parent->id(), GenerateId(), node);
    }
    if (inside_extensive_change_)
      DisableCheck(static_cast<Folder*>(item));
  }
  favorites_[item->id()] = item;
  model_favorites_[node->id()] = item;
  parent->Add(IndexFromModel(parent, index), item);

  if (!thumbnail_path.empty()) {
    file_task_runner_->PostTask(FROM_HERE,
                                base::Bind(&FavoritesImpl::LookForThumbnail,
                                           base::Unretained(this),
                                           item->id(),
                                           thumbnail_path,
                                           importing_from_model_));
  }
}

void FavoritesImpl::RemovePartnerContentAndThumbnail(Favorite* favorite,
                                                     bool skip_parent) {
  const int32_t push_id = FavoritePushId(favorite);
  if (push_id != 0) {
    // If the item we just removed was in a pushed folder then the folder might
    // need to be turned into a non-pushed folder
    if (!skip_parent) {
      UpdatePushedGroupId(favorite->parent());
    }

    if (!favorite->IsFolder()) {
      delegate_->OnPartnerContentRemoved(push_id);
    }
  }
  ScheduleRemoveThumbnail(favorite);
}

void FavoritesImpl::BookmarkNodeRemoved(bookmarks::BookmarkModel* model,
                                        const BookmarkNode* parent,
                                        int old_index,
                                        const BookmarkNode* node,
                                        const std::set<GURL>& removed_urls) {
  DCHECK_EQ(model, bookmark_model_);
  favorites_t::iterator i(model_favorites_.find(node->id()));
  if (i == model_favorites_.end()) return;

  if (node == local_root_->data()) {
    ResetLocalDevice();
    return;
  }

  // Most remove actions done in bookmarkmodel are signaled recursively,
  // but not all of them and this might cause dangling pointers
  int index = node->child_count();
  while (index-- > 0) {
    BookmarkNodeRemoved(
        model, node, index, node->GetChild(index), removed_urls);
  }

  // Clean up parent mapping
  Favorite* favorite = i->second;
  old_index = favorite->GetParent()->IndexOf(favorite);
  favorite->GetParent()->ForgetAbout(favorite);

  // Clean up FavoritesImpl mapping
  model_favorites_.erase(i);
  i = favorites_.find(favorite->id());
  DCHECK(i != favorites_.end());
  favorites_.erase(i);

  // Signal to UI about the changes
  SignalRemoved(favorite->id(), favorite->parent(), old_index, false);

  // Clean up partner and thumbnail meta-data
  RemovePartnerContentAndThumbnail(favorite, false);

  delete favorite;
}

void FavoritesImpl::BookmarkNodeMoved(bookmarks::BookmarkModel* model,
                                      const BookmarkNode* old_parent,
                                      int old_index,
                                      const BookmarkNode* new_parent,
                                      int new_index) {
  DCHECK_EQ(model, bookmark_model_);

  const BookmarkNode* node = new_parent->GetChild(new_index);

  if (node == local_root_->data()) {
    if (new_parent != old_parent) {
      ResetLocalDevice();
      return;
    }
  }

  favorites_t::iterator old_parent_i(model_favorites_.find(old_parent->id()));
  favorites_t::iterator new_parent_i(model_favorites_.find(new_parent->id()));
  if (old_parent_i == model_favorites_.end()) {
    if (new_parent_i != model_favorites_.end()) {
      BookmarkNodeAdded(model, new_parent, new_index);
    }
    return;
  } else {
    if (new_parent_i == model_favorites_.end()) {
      if (node->type() == BookmarkNode::FOLDER) {
        // BookmarkNodeRemoved expects a removed folder to be empty, so make
        // that true. Do not call model->Remove() here as that will confuse
        // everything.
        favorites_t::iterator i = model_favorites_.find(node->id());
        if (i == model_favorites_.end()) return;
        Folder* folder = static_cast<Folder*>(i->second);
        while (folder->Size() > 0) {
          Favorite* child = folder->Child(0);
          RemovePartnerContentAndThumbnail(child, true);
          if (child->data()) {
            favorites_t::iterator j(model_favorites_.find(child->data()->id()));
            if (j != model_favorites_.end()) {
              model_favorites_.erase(j);
            }
          }
          RemoveNonNodeFavorite(child);
        }
      }
      OnWillRemoveBookmarks(model, old_parent, old_index, node);
      BookmarkNodeRemoved(model, old_parent, old_index, node, std::set<GURL>());
      return;
    }
  }

  favorites_t::iterator i(model_favorites_.find(node->id()));
  if (i == model_favorites_.end()) return;

  Folder* current = static_cast<Folder*>(GetFavorite(i->second->parent()));
  Folder* parent = static_cast<Folder*>(new_parent_i->second);
  if (old_parent == new_parent && old_index < new_index &&
      local_add_offset_ < 0) {
    new_index = IndexFromModel(parent, new_index + 1) - 1;
  } else {
    new_index = IndexFromModel(parent, new_index);
  }
  if (current == parent && parent->IndexOf(i->second) == new_index)
    return;
  if (current == parent) {
    parent->Move(parent->IndexOf(i->second), new_index);
  } else {
    parent->Add(new_index, i->second);
  }
}

void FavoritesImpl::OnWillChangeBookmarkNode(bookmarks::BookmarkModel* model,
                                             const BookmarkNode* node) {
  DCHECK_EQ(model, bookmark_model_);
  favorites_t::iterator i(model_favorites_.find(node->id()));
  if (i == model_favorites_.end()) return;

  old_node_url_ = node->url();
  old_node_title_ = node->GetTitle();
}

void FavoritesImpl::BookmarkNodeChanged(bookmarks::BookmarkModel* model,
                                        const BookmarkNode* node) {
  DCHECK_EQ(model, bookmark_model_);
  if (node == local_root_->data()) return;
  favorites_t::iterator i(model_favorites_.find(node->id()));
  if (i == model_favorites_.end()) return;

  Favorite* favorite = i->second;

  unsigned int mask = 0;
  if (node->GetTitle() != old_node_title_) {
    mask |= FavoritesObserver::TITLE_CHANGED;
  }
  if (node->url() != old_node_url_) {
    mask |= FavoritesObserver::URL_CHANGED;
  }
  SignalChanged(favorite->id(), favorite->parent(), mask);

  // Check if partner content favorite was edited by the user,
  // if so report it to the server
  if (update_partner_content_node_ != node &&
      favorite->IsPartnerContent() && !favorite->IsFolder()) {
    if (node->url() != old_node_url_ || node->GetTitle() != old_node_title_) {
      // Fetch push ID before reseting partner data below
      const int32_t push_id = FavoritePushId(favorite);

      // Favorite is no longer considered partner content so make sure
      // it is not treated as such
      BookmarkNode::MetaInfoMap new_map;
      const BookmarkNode::MetaInfoMap* old_map = node->GetMetaInfoMap();
      if (old_map) new_map = *old_map;
      MetaInfo::SetPushedPartnerChannel(new_map, 0);
      MetaInfo::SetPushedPartnerId(new_map, 0);
      MetaInfo::SetPushedPartnerChecksum(new_map, 0);
      bookmark_model_->SetNodeMetaInfoMap(node, new_map);

      // Notify delegate about the edit, make sure this is done after reseting
      // the partner data in case the callback wants to check partner content
      delegate_->OnPartnerContentEdited(push_id);

      // If the item we just edited was in a pushed folder then the folder
      // might need to be turned into a non-pushed folder
      UpdatePushedGroupId(favorite->parent());
    }
  }

  if (node->url() != old_node_url_) {
    DCHECK(!favorite->IsSavedPage());
    RequestThumbnail(favorite);
  }
}

void FavoritesImpl::BookmarkMetaInfoChanged(bookmarks::BookmarkModel* model,
                                            const BookmarkNode* node) {
  DCHECK_EQ(model, bookmark_model_);
  if (node == local_root_->data()) {
    std::string root_guid =
        prefs_->GetString(opera::kBookmarkSpeedDialRootFolderGUIDKey);
    std::string node_guid;
    node->GetMetaInfo(opera::kBookmarkSpeedDialRootFolderGUIDKey, &node_guid);
    if (root_guid != node_guid) {
      // Someone killed our root, act as if all is lost
      ResetLocalDevice();
    }
    return;
  }
  favorites_t::iterator i(model_favorites_.find(node->id()));
  if (i == model_favorites_.end()) return;

  Favorite* favorite = i->second;
  SignalChanged(favorite->id(),
                favorite->parent(),
                FavoritesObserver::METADATA_CHANGED);
}

void FavoritesImpl::BookmarkNodeFaviconChanged(bookmarks::BookmarkModel* model,
                                               const BookmarkNode* node) {
  DCHECK_EQ(model, bookmark_model_);
}

void FavoritesImpl::BookmarkNodeChildrenReordered(
    bookmarks::BookmarkModel* model,
    const BookmarkNode* node) {
  DCHECK_EQ(model, bookmark_model_);
  favorites_t::iterator i(model_favorites_.find(node->id()));
  if (i == model_favorites_.end()) return;
  Folder* folder = static_cast<Folder*>(i->second);
  int folder_index = -1;
  for (int index = 0; index < node->child_count(); index++) {
    const BookmarkNode* node_child = node->GetChild(index);
    Favorite* folder_child;
    do {
      folder_index++;
      folder_child = folder->Child(folder_index);
    } while (folder_child->GetSpecialId() != 0);
    if (folder_child->data() == node_child) continue;
    folder_child = model_favorites_[node_child->id()];
    folder->Move(folder->IndexOf(folder_child), folder_index);
  }
  UpdateSpecialIndexes();
}

void FavoritesImpl::RemoveAllModelChildren(Folder* root) {
  int i = 0;
  while (i < root->Size()) {
    Favorite* child = root->Child(i);
    if (child->IsFolder()) {
      int j = 0;
      Folder* folder = static_cast<Folder*>(child);
      while (j < folder->Size()) {
        Favorite* child_child = folder->Child(j);
        if (child->data() || child_child->data()) {
          if (root == local_root_) {
            RemovePartnerContentAndThumbnail(child_child, true);
          }
          RemoveNonNodeFavorite(child_child);
        } else {
          j++;
        }
      }
    }
    if (child->data()) {
      if (root == local_root_) {
        RemovePartnerContentAndThumbnail(child, true);
      }
      RemoveNonNodeFavorite(child);
    } else {
      i++;
    }
  }
}

void FavoritesImpl::ResetLocalDevice() {
  // Report every node child of local_root_ as removed
  const std::set<GURL> removed_urls;
  auto node = local_root_->data();
  static_cast<RootFolderImpl*>(local_root_)->SetData(NULL);
  for (int i = 0; i < node->child_count(); i++) {
    const BookmarkNode* child = node->GetChild(i);
    if (child->type() == BookmarkNode::FOLDER) {
      for (int j = 0; j < child->child_count(); j++) {
        const BookmarkNode* child_child = child->GetChild(j);
        BookmarkNodeRemoved(bookmark_model_, child, j, child_child,
                            removed_urls);
      }
    }
    BookmarkNodeRemoved(bookmark_model_, node, i, child, removed_urls);
  }

  // Do not report local_root_ as removed, it will be recreated in an instant
  model_favorites_.erase(node->id());

  // To be safe, there should not be any root to find with the current GUID,
  // but if there is we do not want it
  prefs_->ClearPref(opera::kBookmarkSpeedDialRootFolderGUIDKey);
  // If inside extensive change is true we can't recreate the new root now
  // as that would mean BookmarkChangeProcessor would miss the memo. So
  // RemoveDelayed() will do it instead when it sees that local_root_->data()
  // is NULL
  if (!inside_extensive_change_) {
    SetupRoot();
    DCHECK(local_root_->data()->child_count() == 0);
  }
}

void FavoritesImpl::BookmarkAllUserNodesRemoved(
    bookmarks::BookmarkModel* model, const std::set<GURL>& removed_urls) {
  DCHECK_EQ(model, bookmark_model_);
  model_favorites_.clear();
  static_cast<RootFolderImpl*>(devices_root_)->SetData(NULL);
  static_cast<RootFolderImpl*>(local_root_)->SetData(NULL);
  SetupRoot();

  int i = 0;
  while (i < devices_root_->Size()) {
    Folder* device = static_cast<Folder*>(devices_root_->Child(i));
    RemoveAllModelChildren(device);
    if (device == local_root_) {
      local_root_->Check();
      i++;
    } else {
      RemoveNonNodeFavorite(device);
    }
  }
  devices_root_->Check();
}

void FavoritesImpl::ExtensiveBookmarkChangesBeginning(
    bookmarks::BookmarkModel* model) {
  DCHECK_EQ(model, bookmark_model_);
  DCHECK(!inside_extensive_change_);
  inside_extensive_change_ = true;
  // In the unlikely event AfterExtensiveBookmarkChangesEnded hasn't run since
  // the last ExtensiveBookmarkChangesBegin/Ended, don't run DisableCheck
  if (after_extensive_change_posted_)
    return;
  ForEachFolder(base::Bind(&FavoritesImpl::DisableCheck));
}

bool FavoritesImpl::FindFromId(int64_t id, const BookmarkNode* root,
                               bool recursive, const BookmarkNode** parent,
                               int* index) {
  for (int i = 0; i < root->child_count(); ++i) {
    const BookmarkNode* node = root->GetChild(i);
    if (node->id() == id) {
      *parent = root;
      *index = i;
      return true;
    }
    if (recursive && node->is_folder() &&
        FindFromId(id, node, true, parent, index)) {
      return true;
    }
  }
  return false;
}

bool FavoritesImpl::FindFromId(int64_t id, const BookmarkNode** parent,
                               int* index) {
  CHECK(parent);
  CHECK(index);
  return FindFromId(id, local_root_->data(), true, parent, index);
}

void FavoritesImpl::ExtensiveBookmarkChangesEnded(
    bookmarks::BookmarkModel* model) {
  DCHECK_EQ(model, bookmark_model_);
  DCHECK(inside_extensive_change_);
  inside_extensive_change_ = false;
  // Need to delay calling any method that might modify the model a little bit
  // as BookmarkChangeProcessor doesn't observe changes while doing its
  // extensive changes
  // In the unlikely event AfterExtensiveBookmarkChangesEnded hasn't run since
  // the last ExtensiveBookmarkChangesBegin/Ended, don't post another
  if (after_extensive_change_posted_)
    return;
  after_extensive_change_posted_ = true;
  ui_task_runner_->PostTask(
      FROM_HERE, base::Bind(&FavoritesImpl::AfterExtensiveBookmarkChangesEnded,
                            base::Unretained(this)));
}

void FavoritesImpl::AfterExtensiveBookmarkChangesEnded() {
  if (!after_extensive_change_posted_) return;
  DCHECK(!inside_extensive_change_);
  after_extensive_change_posted_ = false;
  ForEachFolder(base::Bind(&FavoritesImpl::RestoreCheck));
  RemoveDelayed();
}

void FavoritesImpl::RemoveDelayed() {
  if (inside_extensive_change_) {
    return;
  }
  for (const int64_t& id : delayed_removal_) {
    const BookmarkNode* parent;
    int index;
    if (FindFromId(id, &parent, &index) ||
        FindFromId(id, devices_root_->data(), false, &parent, &index)) {
      bookmark_model_->Remove(parent->GetChild(index));
    }
  }
  delayed_removal_.clear();
  if (!local_root_->data()) {
    SetupRoot();
    DCHECK(local_root_->data()->child_count() == 0);
  }
}

void FavoritesImpl::Rename(const BookmarkNode* node,
                           const base::string16& title) {
  bookmark_model_->SetTitle(node, title);
}

void FavoritesImpl::ChangeURL(const BookmarkNode* node, const GURL& url) {
  bookmark_model_->SetURL(node, url);
}

void FavoritesImpl::ChangePartnerActivationCount(const BookmarkNode* node,
                                                 int32_t count) {
  BookmarkNode::MetaInfoMap new_map;
  const BookmarkNode::MetaInfoMap* old_map = node->GetMetaInfoMap();
  if (old_map) new_map = *old_map;
  MetaInfo::SetPushedPartnerActivationCount(new_map, count);
  bookmark_model_->SetNodeMetaInfoMap(node, new_map);
}

void FavoritesImpl::ChangePartnerChannel(const BookmarkNode* node,
                                         int32_t channel) {
  BookmarkNode::MetaInfoMap new_map;
  const BookmarkNode::MetaInfoMap* old_map = node->GetMetaInfoMap();
  if (old_map) new_map = *old_map;
  MetaInfo::SetPushedPartnerChannel(new_map, channel);
  bookmark_model_->SetNodeMetaInfoMap(node, new_map);
}

void FavoritesImpl::ChangePartnerId(const BookmarkNode* node, int32_t id) {
  BookmarkNode::MetaInfoMap new_map;
  const BookmarkNode::MetaInfoMap* old_map = node->GetMetaInfoMap();
  if (old_map) new_map = *old_map;
  MetaInfo::SetPushedPartnerId(new_map, id);
  bookmark_model_->SetNodeMetaInfoMap(node, new_map);
}

void FavoritesImpl::ChangePartnerChecksum(const BookmarkNode* node,
                                          int32_t checksum) {
  BookmarkNode::MetaInfoMap new_map;
  const BookmarkNode::MetaInfoMap* old_map = node->GetMetaInfoMap();
  if (old_map) new_map = *old_map;
  MetaInfo::SetPushedPartnerChecksum(new_map, checksum);
  bookmark_model_->SetNodeMetaInfoMap(node, new_map);
}

void FavoritesImpl::ChangePartnerGroup(const BookmarkNode* node,
                                       int32_t group) {
  BookmarkNode::MetaInfoMap new_map;
  const BookmarkNode::MetaInfoMap* old_map = node->GetMetaInfoMap();
  if (old_map) new_map = *old_map;
  MetaInfo::SetPushedPartnerGroupId(new_map, group);
  bookmark_model_->SetNodeMetaInfoMap(node, new_map);
}

std::unique_ptr<opera::SuggestionProvider>
FavoritesImpl::CreateSuggestionProvider() {
  return std::unique_ptr<opera::SuggestionProvider>(
      new FavoriteSuggestionProvider(this));
}

int64_t FavoritesImpl::GetRoot() {
  return local_root_->id();
}

bool FavoritesImpl::IsPartnerContent(int64_t favorite_id) {
  Favorite* favorite = GetFavorite(favorite_id);

  if (favorite) {
    return favorite->IsPartnerContent();
  }

  return false;
}

int64_t FavoritesImpl::GetParent(int64_t favorite_id) {
  Favorite* favorite = GetFavorite(favorite_id);

  if (favorite) {
    return favorite->GetParent()->id();
  }

  // Default to returning the root element
  return GetRoot();
}

int32_t FavoritesImpl::GetPushActivationCount(int64_t favorite_id) {
  Favorite* favorite = GetFavorite(favorite_id);

  if (favorite) {
    return favorite->PartnerActivationCount();
  }

  return 0;
}

int32_t FavoritesImpl::GetPushId(int64_t favorite_id) {
  Favorite* favorite = GetFavorite(favorite_id);

  if (favorite) {
    return FavoritePushId(favorite);
  }

  return 0;
}

int64_t FavoritesImpl::FindEntryFromPushChannel(int32_t channel_id) {
  std::vector<Favorite*> all_favorites = GetAllPartnerContent();

  for (auto i(all_favorites.begin()); i != all_favorites.end(); ++i) {
    if ((*i)->PartnerChannel() == channel_id) {
      return (*i)->id();
    }
  }

  return GetRoot();
}

int64_t FavoritesImpl::FindFolderFromPushId(int32_t push_id) {
  std::vector<Favorite*> all_favorites = GetAllPartnerContentFolders();

  for (auto i(all_favorites.begin()); i != all_favorites.end(); ++i) {
    Folder* folder = static_cast<Folder*>(*i);
    if (folder->PartnerGroup() == push_id) {
      return folder->id();
    }
  }

  return GetRoot();
}

void FavoritesImpl::ResetActivationCountForAllPartnerContent() {
  std::vector<Favorite*> all_favorites = GetAllPartnerContent();

  for (auto i(all_favorites.begin()); i != all_favorites.end(); ++i) {
    (*i)->SetPartnerActivationCount(0);
  }
}

void FavoritesImpl::SetPushData(int64_t favorite_id,
                                int32_t channel,
                                int32_t push_id,
                                int32_t checksum,
                                int32_t activation_count) {
  Favorite* favorite = GetFavorite(favorite_id);

  if (favorite) {
    favorite->SetPartnerChannel(channel);
    favorite->SetPartnerId(push_id);
    favorite->SetPartnerChecksum(checksum);
    favorite->SetPartnerActivationCount(activation_count);
  }
}

int32_t FavoritesImpl::GetChildCount(int64_t folder_id) {
  Favorite* favorite = GetFavorite(folder_id);

  if (favorite && favorite->IsFolder()) {
    Folder* folder = static_cast<Folder*>(favorite);

    return folder->Size();
  }

  return 0;
}

void FavoritesImpl::RemovePartnerContent(int64_t favorite_id) {
  Favorite* favorite = GetFavorite(favorite_id);

  // Remove fields identifying this as a partner entry before removing it. This
  // will result in Bream not registering the entry as a removed partner entry
  if (favorite->IsFolder()) {
    static_cast<Folder*>(favorite)->SetPartnerGroup(0);
  } else {
    favorite->SetPartnerChannel(0);
    favorite->SetPartnerId(0);
  }

  Remove(favorite_id);
}

void FavoritesImpl::UpdatePushedGroupId(int64_t folder_id) {
  if (folder_id == GetRoot())
    return;

  Favorite* favorite = GetFavorite(folder_id);

  if (favorite && favorite->IsFolder()) {
    Folder* folder = static_cast<Folder*>(favorite);

    if (folder->PartnerGroup() != 0) {
      const std::vector<int64_t> children = folder->Children();

      bool pushed_child_found = false;
      for (auto it(children.begin()); it != children.end(); it++) {
        Favorite* child = GetFavorite(*it);
        if (!child) {
          continue;
        }

        if (child->PartnerId() != 0) {
          pushed_child_found = true;
          break;
        }
      }

      if (!pushed_child_found) {
        folder->SetPartnerGroup(0);
      }
    }
  }
}

Favorites* Favorites::instance() {
  static FavoritesImpl* favorites(NULL);
  if (!favorites) {
    favorites = new FavoritesImpl();
  }
  return favorites;
}

std::vector<Favorite*> FavoritesImpl::GetAllPartnerContent() {
  std::vector<Favorite*> favorites;
  for (auto i(favorites_.begin()); i != favorites_.end(); ++i) {
    if (i->second->IsPartnerContent() && !i->second->IsFolder()) {
      favorites.push_back(i->second);
    }
  }
  return favorites;
}

std::vector<Favorite*> FavoritesImpl::GetAllPartnerContentFolders() {
  std::vector<Favorite*> favorites;
  for (auto i(favorites_.begin()); i != favorites_.end(); ++i) {
    if (i->second->IsPartnerContent() && i->second->IsFolder()) {
      favorites.push_back(i->second);
    }
  }
  return favorites;
}

int32_t* FavoritesImpl::GetHeaderDataForAllPartnerContent(size_t* size) {
  std::vector<Favorite*> favorites(GetAllPartnerContent());
  if (favorites.empty()) {
    *size = 0;
    return 0;
  }

  *size = favorites.size() * 3;
  int32_t* data = new int32_t[*size];
  int32_t* o = data;
  for (auto i(favorites.begin()); i != favorites.end(); ++i) {
    *o++ = (*i)->PartnerChannel();
    *o++ = (*i)->PartnerId();
    *o++ = (*i)->PartnerChecksum();
  }

  return data;
}

int32_t* FavoritesImpl::GetUsageHeaderDataForAllPartnerContent(size_t* size) {
  std::vector<Favorite*> favorites(GetAllPartnerContent());

  // Compact the favorites vector by removing all entries with
  // PartnerActivationCount() == 0 from it.
  // This is done by refilling the vector using the o iterator with all
  // entries that have PartnerActivationCount() > 0 and skipping the entries
  // that have PartnerActivationCount() == 0.
  auto o(favorites.begin());
  for (auto i(favorites.begin()); i != favorites.end(); ++i) {
    if ((*i)->PartnerActivationCount() > 0) {
      *o++ = *i;
    }
  }
  favorites.erase(o, favorites.end());

  if (favorites.empty()) {
    *size = 0;
    return 0;
  }

  *size = favorites.size() * 2;
  int32_t* data = new int32_t[*size];
  int32_t* p = data;
  for (auto i(favorites.begin()); i != favorites.end(); ++i) {
    *p++ = (*i)->PartnerId();
    *p++ = (*i)->PartnerActivationCount();
  }

  return data;
}

int64_t FavoritesImpl::CreatePartnerContent(int64_t parent,
                                            int position,
                                            const char* title,
                                            const char* url,
                                            const int8_t* thumbnail,
                                            size_t thumbnail_size,
                                            int32_t partner_channel,
                                            int32_t partner_id,
                                            int32_t partner_checksum) {
  Favorite* favorite = GetFavorite(parent);
  DCHECK(favorite && favorite->IsFolder());
  favorite = CreatePartnerContent(static_cast<Folder*>(favorite),
                                  position,
                                  base::UTF8ToUTF16(title),
                                  AddMissingHTTPIfNeeded(url),
                                  "",
                                  0,
                                  partner_channel,
                                  partner_id,
                                  partner_checksum,
                                  false);
  if (thumbnail && thumbnail_size > 0) {
    SetThumbnailData(favorite, thumbnail, thumbnail_size);
  } else {
    RequestThumbnail(favorite);
  }
  return favorite->id();
}

int64_t FavoritesImpl::CreateFolder(int position,
                                    const char* title,
                                    const int8_t* thumbnail,
                                    size_t thumbnail_size,
                                    int32_t partner_group_id) {
  Folder* folder = CreateFolder(position,
                                base::UTF8ToUTF16(title),
                                partner_group_id);

  if (thumbnail && thumbnail_size > 0) {
    SetThumbnailData(folder, thumbnail, thumbnail_size);
  }

  return folder->id();
}

void FavoritesImpl::UpdatePartnerGroup(int64_t id,
                                       const char* title,
                                       const int8_t* thumbnail,
                                       size_t thumbnail_size) {
  Favorite* favorite = GetFavorite(id);
  if (favorite) {
    const BookmarkNode* node = favorite->data();
    update_partner_content_node_ = node;
    if (title) {
      bookmark_model_->SetTitle(node, base::UTF8ToUTF16(title));
    }
    DCHECK(update_partner_content_node_ == node);
    update_partner_content_node_ = NULL;
    if (thumbnail && thumbnail_size > 0) {
      SetThumbnailData(favorite, thumbnail, thumbnail_size);
    } else {
      ClearThumbnail(id);
    }
  }
}

void FavoritesImpl::ClearThumbnail(int64_t id) {
  Favorite* favorite = GetFavorite(id);
      ScheduleRemoveThumbnail(favorite);
      favorite->SetThumbnailPath("");
    }

void FavoritesImpl::UpdatePartnerContent(int64_t id,
                                         const char* title,
                                         const char* url,
                                         const int8_t* thumbnail,
                                         size_t thumbnail_size) {
  Favorite* favorite = GetFavorite(id);
  if (favorite) {
    const BookmarkNode* node = favorite->data();
    update_partner_content_node_ = node;
    if (title) {
      bookmark_model_->SetTitle(node, base::UTF8ToUTF16(title));
    }
    if (url) {
      bookmark_model_->SetURL(node, AddMissingHTTPIfNeeded(url));
    }
    DCHECK(update_partner_content_node_ == node);
    update_partner_content_node_ = NULL;
    if (thumbnail && thumbnail_size > 0) {
      // Make sure no ongoing thumbnail requests overwrites the new data
      CancelThumbnailRequest(favorite);
      SetThumbnailData(favorite, thumbnail, thumbnail_size);
    } else {
      RequestThumbnail(favorite);
    }
  }
}

void FavoritesImpl::StoreThumbnail(int64_t id,
                                   const std::string& filename,
                                   const std::vector<char>& data) {
  const base::FilePath path(filename);
  int ret = base::WriteFile(path, &data[0], data.size());
  if (ret == static_cast<int>(data.size())) {
    ui_task_runner_->PostTask(FROM_HERE,
                              base::Bind(&FavoritesImpl::OnThumbnailFound,
                                         base::Unretained(this),
                                         id,
                                         filename));
  } else {
    base::DeleteFile(path, false);
    ui_task_runner_->PostTask(FROM_HERE,
                              base::Bind(&FavoritesImpl::OnThumbnailFound,
                                         base::Unretained(this),
                                         id,
                                         ""));
  }
}

void FavoritesImpl::SetThumbnailData(Favorite* favorite,
                                     const void* data,
                                     size_t size) {
  DCHECK(data && size > 0);
  const char* d = reinterpret_cast<const char*>(data);
  file_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&FavoritesImpl::StoreThumbnail,
                 base::Unretained(this),
                 favorite->id(),
                 path_ + GenerateThumbnailName(favorite),
                 std::vector<char>(d, d + size)));
}

std::string FavoritesImpl::GetFavoriteBaseName(const std::string& guid,
                                               int64_t node_id) const {
  DCHECK(!guid.empty() || node_id);
  return "favorite_" + (guid.empty() ? base::Int64ToString(node_id) : guid) +
      path_suffix_;
}

void FavoritesImpl::ScheduleRemoveThumbnail(Favorite* favorite) {
  if (!favorite->thumbnail_path().empty()) {
    file_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&DeleteFileTask,
                   base::FilePath(favorite->thumbnail_path())));
  }
}

void FavoritesImpl::ImportFromFavoriteCollection(
    const scoped_refptr<opera::FavoriteCollection>& collection) {
  // If this method is called before our BookmarkModel is loaded
  // FavoritesImpl will wait with reporting "loaded" until the import
  // is finished
  collection_reader_.reset(new CollectionReader(this, collection, this));
}

void FavoritesImpl::CollectionReaderIsFinished() {
  import_finished_ = true;
  collection_reader_.reset();
  if (loaded_) {
    FinishLoaded();
  }
}

void FavoritesImpl::GetFavoritesMatching(const std::string& query,
                                         size_t max_matches,
                                         std::vector<TitleMatch>* matches) {
  matches->clear();
  std::vector<bookmarks::BookmarkMatch> tmp;
  tmp.reserve(matches->capacity());
  bookmark_model_->GetBookmarksMatching(base::UTF8ToUTF16(query),
                                        max_matches,
                                        &tmp);
  for (auto i(tmp.begin()); i != tmp.end(); ++i) {
    auto j = model_favorites_.find(i->node->id());
    if (j == model_favorites_.end()) continue;
    TitleMatch match;
    match.node = j->second;
    match.positions = i->title_match_positions;
    matches->push_back(match);
  }
}

void FavoritesImpl::GetFavoritesByURL(const GURL& url,
                                      std::vector<int64_t>* matches) {
  matches->clear();
  std::vector<const BookmarkNode*> tmp;
  tmp.reserve(matches->capacity());
  bookmark_model_->GetNodesByURL(url, &tmp);
  for (auto i(tmp.begin()); i != tmp.end(); ++i) {
    auto j = model_favorites_.find((*i)->id());
    if (j == model_favorites_.end()) continue;
    matches->push_back(j->second->id());
  }
}

void FavoritesImpl::CheckPreferences(bool check_bookmark_loaded) {
  if (prefs_) return;
  if (path_.empty() || !file_task_runner_) return;

  PrefServiceFactory factory;
  factory.SetUserPrefsFile(
      base::FilePath(path_).Append(FILE_PATH_LITERAL("favorites.json")),
      file_task_runner_.get());
  scoped_refptr<PrefRegistrySimple> registry(new PrefRegistrySimple());
  registry->RegisterStringPref(opera::kBookmarkSpeedDialRootFolderGUIDKey, "");
  registry->RegisterStringPref(kOldRootGUIDKey, "");
  pref_registry_ = registry;
  prefs_ = factory.Create(registry.get());

  if (check_bookmark_loaded && bookmark_model_->loaded()) {
    BookmarkModelLoaded(bookmark_model_, false);
  }
}

int FavoritesImpl::GetSpecialIndex(int32_t special) {
  DCHECK(special > 0);
  if (special <= 0 || !local_root_->data()) {
    return -1;
  }
  int index = MetaInfo::GetSpecialIndex(local_root_->data(), special);
  if (index > local_root_->Size()) {
    return local_root_->Size();
  }
  return index;
}

void FavoritesImpl::SetSpecialIndex(int32_t special, int index) {
  DCHECK(special > 0);
  if (special <= 0 || !local_root_->data()) {
    return;
  }
  const BookmarkNode::MetaInfoMap* old_map =
      local_root_->data()->GetMetaInfoMap();
  if (!old_map && index < 0) return;
  BookmarkNode::MetaInfoMap new_map;
  if (old_map) new_map = *old_map;
  MetaInfo::SetSpecialIndex(new_map, special, index);
  bookmark_model_->SetNodeMetaInfoMap(local_root_->data(), new_map);
}

void FavoritesImpl::UpdateSpecialIndexes() {
  const BookmarkNode::MetaInfoMap* old_map =
      local_root_->data()->GetMetaInfoMap();
  BookmarkNode::MetaInfoMap new_map;
  if (old_map) new_map = *old_map;
  const int bookmarks_index = bookmarks_folder_ ?
      local_root_->IndexOf(bookmarks_folder_) : INT_MAX;
  const int saved_pages_index = saved_pages_ ?
      local_root_->IndexOf(saved_pages_) : INT_MAX;
  const int feeds_index = feeds_ ? local_root_->IndexOf(feeds_) : INT_MAX;

  if (bookmarks_folder_) {
    // Correct for bookmarks always being created first at load
    int index = bookmarks_index;
    if (saved_pages_index < bookmarks_index) index--;
    if (feeds_index < bookmarks_index) index--;
    MetaInfo::SetSpecialIndex(new_map, bookmarks_folder_->GetSpecialId(),
                              index);
  }
  if (saved_pages_) {
    // Correct for saved pages always being created after bookmarks but
    // before feeds at load
    int index = saved_pages_index;
    if (feeds_index < saved_pages_index) index--;
    MetaInfo::SetSpecialIndex(new_map, saved_pages_->GetSpecialId(), index);
  }
  if (feeds_) {
    MetaInfo::SetSpecialIndex(new_map, feeds_->GetSpecialId(), feeds_index);
  }
  bookmark_model_->SetNodeMetaInfoMap(local_root_->data(), new_map);
}

int FavoritesImpl::IndexFromModel(Folder* parent, int index) {
  if (parent != local_root_) return index;
  for (int i = 0; i < parent->Size() && i < index; i++) {
    if (parent->Child(i)->GetSpecialId() != 0) {
      index++;
    }
  }
  if (local_add_offset_ > 0) {
    index += local_add_offset_;
  }
  return index;
}

int FavoritesImpl::IndexToModel(Folder* parent, int index) {
  if (parent != local_root_) return index;
  int ret = 0, offset = 0;
  for (int i = 0; i < index; i++) {
    if (parent->Child(i)->GetSpecialId() == 0) {
      ret++;
      offset = 0;
    } else {
      offset++;
    }
  }
  DCHECK(local_add_offset_ <= 0);
  if (local_add_offset_ == 0) {
    local_add_offset_ = offset;
  }
  return ret;
}

void FavoritesImpl::BeginLocalAdd() {
  DCHECK(local_add_offset_ < 0);
  local_add_offset_ = 0;
}

void FavoritesImpl::EndLocalAdd() {
  DCHECK(local_add_offset_ >= 0);
  local_add_offset_ = -1;
}

void FavoritesImpl::ProtectRootFromSyncMerge() {
  DCHECK(local_root_->data());
  if (!local_root_->data()) return;

  if (local_root_->data()->sync_transaction_version() ==
      BookmarkNode::kInvalidSyncTransactionVersion) {
    // Root has never synced so no need to protect
    return;
  }

  const BookmarkNode::MetaInfoMap* old_map =
      local_root_->data()->GetMetaInfoMap();
  std::string root_guid = base::GenerateGUID();
  prefs_->SetString(opera::kBookmarkSpeedDialRootFolderGUIDKey, root_guid);
  BookmarkNode::MetaInfoMap new_map;
  if (old_map) {
    auto it = old_map->find(opera::kBookmarkSpeedDialRootFolderGUIDKey);
    if (it != old_map->end()) {
      prefs_->SetString(kOldRootGUIDKey, it->second);
    }
    new_map = *old_map;
  }
  prefs_->CommitPendingWrite();
  new_map[opera::kBookmarkSpeedDialRootFolderGUIDKey] = root_guid;
  bookmark_model_->SetNodeMetaInfoMap(local_root_->data(), new_map);
}

void FavoritesImpl::ForEachFolder(
    const base::Callback<void(Folder*)>& callback) {
  for (int i = 0; i < devices_root_->Size(); i++) {
    Favorite* device = devices_root_->Child(i);
    if (device->IsFolder()) {
      Folder* root = static_cast<Folder*>(device);
      for (int j = 0; j < root->Size(); j++) {
        Favorite* child = root->Child(j);
        if (child->IsFolder()) {
          callback.Run(static_cast<Folder*>(child));
        }
      }
      callback.Run(root);
    }
  }
  callback.Run(devices_root_);
}

// static
void FavoritesImpl::DisableCheck(Folder* folder) {
  folder->DisableCheck();
}

// static
void FavoritesImpl::RestoreCheck(Folder* folder) {
  folder->RestoreCheckWithoutAction();
}

void FavoritesImpl::RemoveRemoteDevices() {
  if (bookmark_model_ && local_root_->data()) {
    opera::OperaBookmarkUtils::RemoveOtherDevices(
        bookmark_model_, local_root_->data());
  }
}

}  // namespace mobile
