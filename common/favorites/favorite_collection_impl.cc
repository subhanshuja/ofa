// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/favorites/favorite_collection_impl.h"

#include <vector>

#include "base/bind.h"
#include "base/location.h"
#include "base/strings/utf_string_conversions.h"

#include "common/favorites/favorite_collection_observer.h"
#include "common/favorites/favorite_storage.h"
#include "common/favorites/favorite_utils.h"

namespace {

void IndexFavorite(opera::FavoriteIndex* index,
                   opera::Favorite* favorite,
                   int /* ignored */) {
  index->Add(favorite);
}

void PushFavoriteDataToStorage(
    const scoped_refptr <opera::FavoriteStorage::FavoriteRawDataHolder>& holder,
    opera::Favorite* favorite,
    int favorite_index) {
  if (favorite->IsTransient())
    return;

  base::Pickle serialization_data;
  favorite->data().Serialize(&serialization_data);

  opera::FavoriteFolder* parent = favorite->parent();

  holder->PushFavoriteRawData(
      favorite->guid(),
      favorite_index,
      favorite->title(),
      favorite->display_url().spec(),
      favorite->GetType(),
      parent ? parent->guid() : "",
      serialization_data);
}

}  // namespace

namespace opera {

const int FavoriteCollection::kIndexPositionLast = -1;

FavoriteCollectionImpl::FavoriteCollectionImpl(
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
  FavoriteStorage* storage,
  std::unique_ptr<PartnerLogicDelegate> partner_logic_delegate)
  : weak_ptr_factory_(this),
    partner_logic_delegate_(std::move(partner_logic_delegate)),
    index_(new FavoriteIndex),
    data_store_(storage),
    ui_task_runner_(ui_task_runner),
    is_saving_(false),
    storage_dirty_(false) {
  if (partner_logic_delegate_ != NULL)
    partner_logic_delegate_->set_favorite_collection(this);
  Load();
}

FavoriteCollectionImpl::~FavoriteCollectionImpl() {
  data_store_->Stop();
  UpdatePersistentStorageOnExit();
}

void FavoriteCollectionImpl::AddObserver(FavoriteCollectionObserver* observer) {
  observer_list_.AddObserver(observer);
}

void FavoriteCollectionImpl::RemoveObserver(
    FavoriteCollectionObserver* observer) {
  observer_list_.RemoveObserver(observer);
}

void FavoriteCollectionImpl::Load() {
  data_store_->Load(base::Bind(&FavoriteCollectionImpl::CollectionLoaded,
                               base::Unretained(this)));
}

void FavoriteCollectionImpl::Flush() {
  if (!root_folder_ || (!storage_dirty_ && pending_save_.is_null()))
    return;

  SaveToStorage(false);
  data_store_->Flush();
}

bool FavoriteCollectionImpl::IsLoaded() const {
  return root_folder_ != NULL;
}

FavoriteSite* FavoriteCollectionImpl::AddFavoriteSite(const GURL& url,
                                                      const base::string16& name,
                                                      FavoriteFolder* parent,
                                                      int index,
                                                      const std::string& guid,
                                                      ChangeReason reason) {
  if (!IsLoaded())
    return NULL;
  if (!guid.empty() && FindByGUID(guid))
    return NULL;
  if (!root_folder_->CanAddChildAt(parent, index))
    return NULL;

  FavoriteData data;
  data.title = name;
  data.url = url;
  FavoriteSite* fav = new FavoriteSite(guid, data);

  if (partner_logic_delegate_ != NULL)
    partner_logic_delegate_->AddOrMoveFavorite(fav, parent, reason);

  root_folder_->AddChildAt(parent, fav, &index);

  OnFavoriteAdded(fav, index, reason);
  return fav;
}

FavoriteExtension* FavoriteCollectionImpl::AddFavoriteExtension(
    const std::string& extension_id,
    const std::string& extension,
    FavoriteExtensionState state,
    FavoriteFolder* parent,
    int index,
    ChangeReason reason) {
  if (!IsLoaded())
    return NULL;
  if (!root_folder_->CanAddChildAt(parent, index))
    return NULL;

  FavoriteData data;
  data.extension_state = state;
  data.extension = extension;
  FavoriteExtension* fav = new FavoriteExtension(extension_id, data, false);

  if (partner_logic_delegate_ != NULL)
    partner_logic_delegate_->AddOrMoveFavorite(fav, parent, reason);

  root_folder_->AddChildAt(parent, fav, &index);

  OnFavoriteAdded(fav, index, reason);
  return fav;
}

FavoriteSite* FavoriteCollectionImpl::AddFavoritePartnerContent(
    const GURL& url,
    const base::string16& name,
    FavoriteFolder* parent,
    const std::string& guid,
    const std::string& partner_id,
    bool redirect,
    const FavoriteKeywords& custom_keywords,
    int index,
    ChangeReason reason) {
  if (!IsLoaded())
    return NULL;
  if (!guid.empty() && FindByGUID(guid))
    return NULL;
  if (!root_folder_->CanAddChildAt(parent, index))
    return NULL;

  FavoriteData data;
  data.title = name;
  data.url = url;
  data.partner_id = partner_id;
  data.redirect = redirect;
  data.custom_keywords = custom_keywords;

  FavoriteSite* fav = new FavoriteSite(guid, data);

  if (partner_logic_delegate_ != NULL)
    partner_logic_delegate_->AddOrMoveFavorite(fav, parent, reason);

  root_folder_->AddChildAt(parent, fav, &index);

  OnFavoriteAdded(fav, index, reason);
  return fav;
}

FavoriteSite* FavoriteCollectionImpl::AddFavoritePartnerContent(
    const GURL& url,
    const base::string16& name,
    FavoriteFolder* parent,
    const std::string& guid,
    int pushed_partner_activation_count,
    int pushed_partner_channel,
    int pushed_partner_id,
    int pushed_partner_checksum,
    int index,
    ChangeReason reason) {
  if (!IsLoaded())
    return NULL;
  if (!guid.empty() && FindByGUID(guid))
    return NULL;
  if (!root_folder_->CanAddChildAt(parent, index))
    return NULL;

  FavoriteData data;
  data.title = name;
  data.url = url;
  data.pushed_partner_activation_count = pushed_partner_activation_count;
  data.pushed_partner_channel = pushed_partner_channel;
  data.pushed_partner_id = pushed_partner_id;
  data.pushed_partner_checksum = pushed_partner_checksum;

  FavoriteSite* fav = new FavoriteSite(guid, data);

  if (partner_logic_delegate_ != NULL)
    partner_logic_delegate_->AddOrMoveFavorite(fav, parent, reason);

  root_folder_->AddChildAt(parent, fav, &index);

  OnFavoriteAdded(fav, index, reason);
  return fav;
}

FavoriteFolder* FavoriteCollectionImpl::AddFavoriteFolder(
    const base::string16& name,
    int index,
    const std::string& guid,
    ChangeReason reason) {
  if (!IsLoaded())
    return NULL;
  if (!guid.empty() && FindByGUID(guid))
    return NULL;
  if (!root_folder_->CanAddChildAt(NULL, index))
    return NULL;

  FavoriteData data;
  data.title = name;

  FavoriteFolder* folder = new FavoriteFolder(guid, data);
  root_folder_->AddChildAt(NULL, folder, &index);

  OnFavoriteAdded(folder, index, reason);
  return folder;
}

FavoriteFolder* FavoriteCollectionImpl::AddPartnerContentFolder(
    const base::string16& name,
    int pushed_partner_group_id,
    int index,
    const std::string& guid,
    ChangeReason reason) {
  if (!IsLoaded())
    return NULL;
  if (!guid.empty() && FindByGUID(guid))
    return NULL;
  if (!root_folder_->CanAddChildAt(NULL, index))
    return NULL;

  FavoriteData data;
  data.title = name;
  data.pushed_partner_group_id = pushed_partner_group_id;

  FavoriteFolder* folder = new FavoriteFolder(guid, data);
  root_folder_->AddChildAt(NULL, folder, &index);

  OnFavoriteAdded(folder, index, reason);
  return folder;
}

FavoriteFolder* FavoriteCollectionImpl::AddFavoritePlaceholderFolder(
    const base::string16& name,
    int index,
    const std::string& guid,
    ChangeReason reason) {
  if (!IsLoaded())
    return NULL;
  if (!guid.empty() && FindByGUID(guid))
    return NULL;
  if (!root_folder_->CanAddChildAt(NULL, index))
    return NULL;

  FavoriteData data;
  data.title = name;
  data.is_placeholder_folder = true;

  FavoriteFolder* folder = new FavoriteFolder(guid, data);
  root_folder_->AddChildAt(NULL, folder, &index);

  OnFavoriteAdded(folder, index, reason);
  return folder;
}

void FavoriteCollectionImpl::RemoveFavorite(Favorite* favorite,
                                            ChangeReason reason) {
  DCHECK(favorite);

  if (!IsLoaded())
    return;

  if (favorite->GetType() == Favorite::kFavoriteFolder) {
    DCHECK(!favorite->parent());
    FavoriteFolder* removed_folder = static_cast<FavoriteFolder*>(favorite);
    if (removed_folder->GetChildCount() > 0) {
      VLOG(5) << "Fav. collection was told to remove a favorite folder that is"
              << " not empty. Removing all the children also.";
      do {
        RemoveFavorite(removed_folder->GetChildByIndex(0), reason);
      } while (removed_folder->GetChildCount() > 0);
    }
  }

  OnBeforeFavoriteRemove(favorite,
                         favorite->parent(),
                         root_folder_->GetIndexOf(favorite),
                         reason);
  std::string favorite_guid = favorite->guid();
  root_folder_->Remove(favorite);
  OnAfterFavoriteRemoved(favorite_guid, reason);

  UpdatePersistentStorage();
}

bool FavoriteCollectionImpl::MoveFavorite(Favorite* favorite,
                                          FavoriteFolder* dest_folder,
                                          int new_index,
                                          ChangeReason reason) {
  if (!IsLoaded())
    return false;

  if (!root_folder_->CanMove(favorite, dest_folder, new_index))
    return false;

  FavoriteFolder* old_parent = favorite->parent();
  int old_index = old_parent ? old_parent->GetIndexOf(favorite) :
                               root_folder_->GetIndexOf(favorite);

  if (partner_logic_delegate_ != NULL)
    partner_logic_delegate_->AddOrMoveFavorite(favorite, dest_folder, reason);

  root_folder_->Move(favorite, dest_folder, old_index, new_index);

  OnFavoriteMoved(favorite,
                  old_parent,
                  old_index,
                  favorite->parent(),
                  root_folder_->GetIndexOf(favorite),
                  reason);
  return true;
}

bool FavoriteCollectionImpl::MoveFavorite(Favorite* favorite,
                                          FavoriteFolder* new_parent,
                                          const std::string& prev_guid,
                                          ChangeReason reason) {
  if (!IsLoaded())
    return false;

  if (!root_folder_->CanMove(favorite, new_parent, prev_guid))
    return false;

  FavoriteFolder* old_parent = favorite->parent();
  int old_index = old_parent ? old_parent->GetIndexOf(favorite) :
                               root_folder_->GetIndexOf(favorite);
  int new_index;

  if (partner_logic_delegate_ != NULL)
    partner_logic_delegate_->AddOrMoveFavorite(favorite, new_parent, reason);

  root_folder_->Move(favorite, new_parent, prev_guid, &new_index);

  OnFavoriteMoved(favorite,
                  old_parent,
                  old_index,
                  favorite->parent(),
                  new_index,
                  reason);

  return true;
}

void FavoriteCollectionImpl::ChangeFavorite(
    Favorite* favorite,
    const FavoriteData& new_data,
    ChangeReason reason) {
  if (!IsLoaded())
    return;

  DCHECK(favorite);

  const FavoriteData old_data(favorite->data_);

  if (old_data.url != new_data.url)
    RemoveFromOrderedByURLSet(favorite);

  favorite->data_ = new_data;

  index_->Remove(favorite);
  index_->Add(favorite);

  if (partner_logic_delegate_ != NULL)
    partner_logic_delegate_->HandleChange(favorite, old_data, reason);

  FOR_EACH_OBSERVER(FavoriteCollectionObserver,
                    observer_list_,
                    FavoriteChanged(favorite, old_data, reason));

  if (old_data.url != new_data.url)
    AddToOrderedByURLSet(favorite);

  UpdatePersistentStorage();
}

void FavoriteCollectionImpl::GetFavoritesByURL(
    const GURL& url,
    std::vector<const Favorite*>* favorites) {
  base::AutoLock lock(favorites_ordered_by_url_lock_);
  FavoriteData data;
  data.url = url;
  FavoriteSite tmp_node(std::string(), data);
  FavoritesOrderedByURLSet::iterator i =
      favorites_ordered_by_url_set_.find(&tmp_node);

  while (i != favorites_ordered_by_url_set_.end()) {
    if ((*i)->display_url() != url)
      break;

    favorites->push_back(*i);
    ++i;
  }
}

int FavoriteCollectionImpl::GetCount() const {
  if (!IsLoaded())
    return 0;

  return root_folder_->GetChildCount();
}

Favorite* FavoriteCollectionImpl::GetAt(int index) const {
  if (!IsLoaded())
    return NULL;

  return root_folder_->GetChildByIndex(index);
}

int FavoriteCollectionImpl::GetIndexOf(const Favorite* favorite) const {
  if (!IsLoaded())
    return -1;

  return root_folder_->GetIndexOf(favorite);
}

Favorite* FavoriteCollectionImpl::FindByGUID(const std::string& guid) const {
  if (!IsLoaded())
    return NULL;

  for (int i = 0; i < root_folder_->GetChildCount(); ++i) {
    Favorite* favorite = root_folder_->GetChildByIndex(i);
      if (favorite->guid() == guid)
        return favorite;

    if (favorite->GetType() == Favorite::kFavoriteFolder) {
      Favorite* found_favorite =
          static_cast<FavoriteFolder*>(favorite)->FindByGUID(guid);
      if (found_favorite)
        return found_favorite;
    }
  }
  return NULL;
}

void FavoriteCollectionImpl::GetFavoritesWithTitlesMatching(
    const base::string16& query,
    size_t max_count,
    std::vector<FavoriteUtils::TitleMatch>* matches) {
  index_->GetFavoritesWithTitlesMatching(query, max_count, matches);
}


int FavoriteCollectionImpl::FilterFavoritePartnerContentFromFolder(
    std::vector<Favorite*>* to_remove,
    FavoriteFolder* folder,
    const base::hash_set<std::string>& to_stay) {
  int number_of_children_to_remove = 0;
  for (int i = 0; i < folder->GetChildCount(); ++i) {
    Favorite* removal_candidate = folder->GetChildByIndex(i);
    if (removal_candidate->GetType() == Favorite::kFavoriteSite) {
      if (removal_candidate->is_placeholder_folder() ||
          to_stay.find(removal_candidate->guid()) != to_stay.end())
        continue;  // Cant't touch this.
      // Hammer time!
      to_remove->push_back(removal_candidate);
      ++number_of_children_to_remove;
    } else if (removal_candidate->GetType() == Favorite::kFavoriteFolder) {
      // This is a folder. Filter its children (recursively) and if the folder
      // becomes empty afterwards, remove it as well.
      FavoriteFolder* subfolder =
          static_cast<FavoriteFolder*>(removal_candidate);
      int children_removed_from_subfolder =
          FilterFavoritePartnerContentFromFolder(to_remove, subfolder, to_stay);
      if (subfolder->GetChildCount() == children_removed_from_subfolder) {
        to_remove->push_back(subfolder);
        ++number_of_children_to_remove;
      }
    }
  }
  // Note that we can't return to_remove.size() as that's a collection shared
  // between recursive calls. We want to return the number of children added to
  // to_remove within this call.
  return number_of_children_to_remove;
}

void FavoriteCollectionImpl::FilterFavoritePartnerContent(
    const base::hash_set<std::string>& to_stay) {
  // Removing Favorites while iterating over the collection is tricky and
  // error prone. We will thus produce a vector of Favorites to remove and
  // then do a second pass.
  std::vector<Favorite*> to_remove;
  FilterFavoritePartnerContentFromFolder(&to_remove,
                                         root_folder_.get(),
                                         to_stay);
  for (std::vector<Favorite*>::iterator i = to_remove.begin();
      i != to_remove.end();
      ++i)
    RemoveFavorite(*i, kChangeReasonOther);
}

FavoriteFolder* FavoriteCollectionImpl::GetRoot() const {
  return root_folder_.get();
}

void FavoriteCollectionImpl::PopulateFavoritesByURL(
    const FavoriteRootFolder* root) {
  DCHECK(!favorites_ordered_by_url_set_.size()) << "List already populated.";
  base::AutoLock lock(favorites_ordered_by_url_lock_);

  for (int i = 0; i < root->GetChildCount(); ++i) {
    const Favorite* node = root->GetChildByIndex(i);

    if (!node->display_url().is_empty() &&
        node->AllowURLIndexing())
      favorites_ordered_by_url_set_.insert(node);

    if (node->GetType() == Favorite::kFavoriteFolder) {
      const FavoriteFolder* folder = static_cast<const FavoriteFolder*>(node);
      for (int j = 0; j < folder->GetChildCount(); ++j) {
        node = folder->GetChildByIndex(j);
        if (!node->display_url().is_empty() &&
            node->AllowURLIndexing())
          favorites_ordered_by_url_set_.insert(node);
      }
    }
  }
}

void FavoriteCollectionImpl::OnCollectionLoaded() {
  PopulateFavoritesByURL(root_folder_.get());
  FavoriteUtils::ForEachFavoriteInFolder(
      root_folder_.get(),
      true,
      base::Bind(&IndexFavorite, index_.get()));

  FOR_EACH_OBSERVER(FavoriteCollectionObserver,
                    observer_list_,
                    FavoriteCollectionLoaded());
}

void FavoriteCollectionImpl::OnCollectionDestroyed() {
  FOR_EACH_OBSERVER(FavoriteCollectionObserver,
                    observer_list_,
                    FavoriteCollectionDeleted());
}

void FavoriteCollectionImpl::OnFavoriteAdded(Favorite* favorite,
                                             int new_index,
                                             ChangeReason reason) {
  index_->Add(favorite);

  AddToOrderedByURLSet(favorite);

  FOR_EACH_OBSERVER(FavoriteCollectionObserver,
                    observer_list_,
                    FavoriteAdded(favorite, new_index, reason));

  if (partner_logic_delegate_ != NULL)
    partner_logic_delegate_->OnFavoriteAddedOrMoved(favorite, reason);

  UpdatePersistentStorage();
}

void FavoriteCollectionImpl::OnBeforeFavoriteRemove(
    const Favorite* favorite,
    const FavoriteFolder* parent,
    int index,
    ChangeReason reason) {
  index_->Remove(favorite);

  RemoveFromOrderedByURLSet(favorite);

  FOR_EACH_OBSERVER(FavoriteCollectionObserver,
                    observer_list_,
                    BeforeFavoriteRemove(favorite, parent, index, reason));
}

void FavoriteCollectionImpl::OnAfterFavoriteRemoved(
    const std::string& favorite_guid,
    ChangeReason reason) {
  FOR_EACH_OBSERVER(FavoriteCollectionObserver,
                    observer_list_,
                    AfterFavoriteRemoved(favorite_guid, reason));
}

void FavoriteCollectionImpl::OnFavoriteMoved(
    Favorite* favorite,
    const FavoriteFolder* old_parent,
    int old_index,
    const FavoriteFolder* new_parent,
    int new_index,
    ChangeReason reason) {
  FOR_EACH_OBSERVER(FavoriteCollectionObserver,
                    observer_list_,
                    FavoriteMoved(favorite,
                                  old_parent,
                                  old_index,
                                  new_parent,
                                  new_index,
                                  reason));

  if (partner_logic_delegate_ != NULL)
    partner_logic_delegate_->OnFavoriteAddedOrMoved(favorite, reason);

  UpdatePersistentStorage();
}

void FavoriteCollectionImpl::CollectionLoaded(FavoriteRootFolder* new_root) {
  root_folder_.reset(new_root);

  if (root_folder_)
    OnCollectionLoaded();
}

void FavoriteCollectionImpl::CollectionSaved() {
  is_saving_ = false;

  DCHECK(pending_save_.is_null());

  if (storage_dirty_) {
    storage_dirty_ = false;
    UpdatePersistentStorage();
  }
}

void FavoriteCollectionImpl::UpdatePersistentStorage() {
  if (!root_folder_ || data_store_->IsStopped())
    return;

  if (is_saving_) {
    storage_dirty_ = true;
    return;
  }

  if (pending_save_.is_null()) {
    pending_save_ = base::Bind(&FavoriteCollectionImpl::SaveToStorage,
                               weak_ptr_factory_.GetWeakPtr(), false);
    ui_task_runner_->PostDelayedTask(
        FROM_HERE, pending_save_, base::TimeDelta::FromSeconds(3));
  }
}

void FavoriteCollectionImpl::UpdatePersistentStorageOnExit() {
  DCHECK(data_store_->IsStopped());
  if (!root_folder_ || !storage_dirty_)
    return;

  SaveToStorage(true);
}

void FavoriteCollectionImpl::SaveToStorage(bool force_save_on_exit) {
  pending_save_.Reset();
  is_saving_ = true;

  scoped_refptr<FavoriteStorage::FavoriteRawDataHolder> holder(
      new FavoriteStorage::FavoriteRawDataHolder());

  FavoriteUtils::ForEachFavoriteInFolder(
      root_folder_.get(),
      true,
      base::Bind(&PushFavoriteDataToStorage, holder));

  if (force_save_on_exit) {
    data_store_->SafeSaveFavoriteDataOnExit(holder);
  } else {
    data_store_->SaveFavoriteData(
        holder,
        base::Bind(&FavoriteCollectionImpl::CollectionSaved,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

FavoriteRootFolder::FavoriteRootFolder()
    : FavoriteFolder(std::string(), FavoriteData()) {
}

bool FavoriteRootFolder::CanAddChildAt(FavoriteFolder* parent,
                                       int index) {
  if (index == FavoriteCollection::kIndexPositionLast)
    return true;
  if (index < 0)
    return false;
  FavoriteFolder* real_parent = parent ? parent : this;
  if (static_cast<size_t>(index) > real_parent->children_.size())
    return false;

  return true;
}

void FavoriteRootFolder::AddChildAt(FavoriteFolder* parent,
                                    Favorite* child,
                                    int* index) {
  FavoriteFolder* real_parent = parent ? parent : this;

  if (*index == FavoriteCollection::kIndexPositionLast)
    *index = real_parent->GetChildCount();

  real_parent->AddChildAt(child, *index);

  if (!parent || parent == this)
    child->set_parent(NULL);
}

void FavoriteRootFolder::Remove(Favorite* favorite) {
  GetRealParent(favorite)->RemoveChild(favorite);
  delete favorite;
}

bool FavoriteRootFolder::CanMove(Favorite* favorite,
                                 FavoriteFolder* target_folder,
                                 int target_index) {
  FavoriteFolder* real_target_parent = target_folder ? target_folder : this;
  int real_index = target_index == FavoriteCollection::kIndexPositionLast
      ? real_target_parent->GetChildCount()
      : target_index;
  if (real_index < 0)
    return false;
  bool same_folder = target_folder == favorite->parent();
  if (same_folder) {
    if (real_index == real_target_parent->GetIndexOf(favorite))
      return false;
    if (real_index >= real_target_parent->GetChildCount())
      // Weak inequality, because if moving within the same folder, we count
      // the index as if the folder doesn't contain the moved favorite.
      return false;
  } else if (real_index > real_target_parent->GetChildCount()) {
    return false;
  }

  return true;
}

void FavoriteRootFolder::Move(Favorite* favorite,
                              FavoriteFolder* move_to_folder,
                              int move_from_index,
                              int move_to_index) {
  FavoriteFolder* real_target = move_to_folder ? move_to_folder : this;

  if (move_to_index == FavoriteCollection::kIndexPositionLast)
    move_to_index = real_target->GetChildCount();

  GetRealParent(favorite)->MoveChildToFolder(
      favorite,
      real_target,
      move_from_index,
      move_to_index);

  // Move to (or within) root folder was requested.
  if (!move_to_folder || move_to_folder == this)
    favorite->set_parent(NULL);
}

bool FavoriteRootFolder::CanMove(Favorite* favorite,
                                 FavoriteFolder* new_parent,
                                 const std::string& prev_guid) {
  if (!prev_guid.empty()) {
    FavoriteFolder* real_new_parent = new_parent ? new_parent : this;
    Favorite* prev_favorite = real_new_parent->FindByGUID(prev_guid);
    if (!prev_favorite || favorite == prev_favorite)
      return false;
  }

  return true;
}

void FavoriteRootFolder::Move(Favorite* favorite,
                              FavoriteFolder* new_parent,
                              const std::string& prev_guid,
                              int* new_index) {
  FavoriteFolder* real_new_parent = new_parent ? new_parent : this;
  Favorite* prev_favorite;
  if (prev_guid.empty()) {
    prev_favorite = NULL;
  } else {
    prev_favorite = real_new_parent->FindByGUID(prev_guid);
    DCHECK(prev_favorite && favorite != prev_favorite);
  }

  GetRealParent(favorite)->RemoveChild(favorite);

  if (prev_favorite) {
    *new_index = real_new_parent->GetIndexOf(prev_favorite);
    DCHECK_NE(-1, *new_index);
    (*new_index)++;
  } else {
    *new_index = 0;
  }

  real_new_parent->AddChildAt(favorite, *new_index);

  // Move to (or within) root folder was requested.
  if (!new_parent || new_parent == this)
    favorite->set_parent(NULL);
}

int FavoriteRootFolder::GetIndexOf(const Favorite* favorite) const {
  return GetRealParent(favorite)->GetIndexOf(favorite);
}

int FavoriteRootFolder::GetChildCount() const {
  return FavoriteFolder::GetChildCount();
}

Favorite* FavoriteRootFolder::GetChildByIndex(int index) const {
  return FavoriteFolder::GetChildByIndex(index);
}

FavoriteFolder* FavoriteRootFolder::GetRealParent(const Favorite* favorite) {
  return favorite->parent() ? favorite->parent() : this;
}

const FavoriteFolder* FavoriteRootFolder::GetRealParent(
    const Favorite* favorite) const {
  return favorite->parent() ? favorite->parent() : this;
}

bool FavoriteRootFolder::RecreateFavorite(
    const std::string& guid,
    const base::string16& name,
    const std::string& url,
    int type,
    const std::string& parent_guid,
    const base::Pickle& serialized_data) {
  Favorite* fav = NULL;

  FavoriteData data;
  data.title = name;
  data.url = GURL(url);
  data.Deserialize(serialized_data);

  switch (type) {
    case Favorite::kFavoriteSite:
      fav = new FavoriteSite(guid, data);
      break;

    case Favorite::kFavoriteFolder:
      fav = new FavoriteFolder(guid, data);
      break;

    case Favorite::kFavoriteExtension:
      fav = new FavoriteExtension(guid, data, false);
      break;

    default:
      DLOG(ERROR) << "Unknown favorite type";
      break;
  }

  if (fav == NULL)
    return false;

  int index = FavoriteCollection::kIndexPositionLast;

  AddChildAt(FindByGUID(parent_guid), fav, &index);

  return true;
}

FavoriteFolder* FavoriteRootFolder::FindByGUID(const std::string& guid) const {
  if (guid.length())
    for (int i = 0; i < GetChildCount(); ++i) {
      Favorite* child = GetChildByIndex(i);
      if (child->guid() == guid) {
        DCHECK(child->GetType() == Favorite::kFavoriteFolder);
        return static_cast<FavoriteFolder*>(child);
      }
    }

  return NULL;
}

bool FavoriteCollectionImpl::FavoriteURLComparator::operator()(
    const Favorite* n1,
    const Favorite* n2) const {
  return n1->display_url() < n2->display_url();
}

void FavoriteCollectionImpl::AddToOrderedByURLSet(const Favorite* favorite) {
  if (!favorite->display_url().is_empty() &&
      favorite->AllowURLIndexing()) {
    base::AutoLock lock(favorites_ordered_by_url_lock_);
    favorites_ordered_by_url_set_.insert(favorite);
  }
}

void FavoriteCollectionImpl::RemoveFromOrderedByURLSet(
    const Favorite* favorite) {
  if (!favorite->display_url().is_empty() &&
      favorite->AllowURLIndexing() &&
      !favorites_ordered_by_url_set_.empty()) {
    base::AutoLock lock(favorites_ordered_by_url_lock_);
    FavoritesOrderedByURLSet::iterator i =
        favorites_ordered_by_url_set_.find(favorite);
    if (i != favorites_ordered_by_url_set_.end()) {
      // i points to the first node with the URL, advance until we find the
      // node we're removing.
      while (*i != favorite)
        ++i;

      favorites_ordered_by_url_set_.erase(i);
    }
  }
}

}  // namespace opera
