// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_FAVORITES_IMPL_H_
#define MOBILE_COMMON_FAVORITES_FAVORITES_IMPL_H_

#include <map>
#include <set>
#include <string>
#include <vector>
#include <utility>

#include "base/observer_list.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string16.h"
#include "components/bookmarks/browser/bookmark_model_observer.h"
#include "url/gurl.h"

#include "mobile/common/favorites/collection_reader.h"
#include "mobile/common/favorites/favorites.h"
#include "mobile/common/favorites/savedpages.h"

namespace bookmarks {
class BookmarkModel;
class BookmarkNode;
}

class PrefRegistry;
class PrefService;

namespace mobile {

class FolderImpl;
struct TitleMatch;

class FavoritesImpl : public Favorites,
                      private FavoritesDelegate::ThumbnailReceiver,
                      private FavoritesDelegate::ImportDelegate,
                      private FavoritesDelegate::PartnerContentInterface,
                      private bookmarks::BookmarkModelObserver,
                      private SavedPagesReader::Delegate,
                      private CollectionReader::Observer {
 public:
  FavoritesImpl();
  virtual ~FavoritesImpl();

  /**
   * @name Favorites implementation
   * @{
   */
  bool IsReady() override;
  bool IsLoaded() override;
  Folder* devices_root() override;
  Folder* local_root() override;
  Folder* bookmarks_folder() override;
  Folder* saved_pages() override;
  Folder* feeds() override;
  void SetBookmarksFolderTitle(const base::string16& title) override;
  void SetSavedPagesTitle(const base::string16& title) override;
  void SetFeedsTitle(const base::string16& title) override;
  void SetRequestGraphicsSizes(int icon_size, int thumb_size) override;
  void SetBaseDirectory(const std::string& path) override;
  void SetSavedPageDirectory(const std::string& path) override;
  void SetDelegate(FavoritesDelegate* delegate) override;
  void SetModel(
      bookmarks::BookmarkModel* bookmark_model,
      const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner,
      const scoped_refptr<base::SingleThreadTaskRunner>& file_task_runner)
      override;
  void SetDeviceName(const std::string& name) override;
  void SetBookmarksEnabled(bool enabled) override;
  void SetSavedPagesEnabled(SavedPagesMode mode) override;
  void SetFeedsEnabled(bool enabled) override;
  Folder* CreateFolder(int pos, const base::string16& title) override;
  Folder* CreateFolder(int pos,
                       const base::string16& title,
                       int32_t pushed_partner_group_id) override;
  Favorite* CreateFavorite(Folder* parent,
                           int pos,
                           const base::string16& title,
                           const GURL& url) override;
  SavedPage* CreateSavedPage(const base::string16& title,
                             const GURL& url,
                             const std::string& file) override;
  void Remove(int64_t id) override;
  void ClearThumbnail(int64_t id) override;
  Favorite* GetFavorite(int64_t id) override;
  void AddObserver(FavoritesObserver* observer) override;
  void RemoveObserver(FavoritesObserver* observer) override;
  std::unique_ptr<opera::SuggestionProvider> CreateSuggestionProvider()
      override;
  void ImportFromFavoriteCollection(
      const scoped_refptr<opera::FavoriteCollection>& collection) override;
  void Flush() override;
  void GetFavoritesMatching(const std::string& query,
                            size_t max_matches,
                            std::vector<TitleMatch>* matches) override;
  void GetFavoritesByURL(const GURL& url,
                         std::vector<int64_t>* matches) override;
  void ProtectRootFromSyncMerge() override;
  void RemoveRemoteDevices() override;

  /** @} */

  void SignalAdded(int64_t id, int64_t parent, int pos);
  void SignalMoved(int64_t id,
                   int64_t old_parent,
                   int old_pos,
                   int64_t new_parent,
                   int new_pos);
  void SignalRemoved(int64_t id,
                     int64_t old_parent,
                     int old_pos,
                     bool recursive);
  void SignalChanged(int64_t id, int64_t parent, unsigned int changed);

  void RequestThumbnail(Favorite* favorite);

  void CancelThumbnailRequest(Favorite* favorite);

  void ReportPartnerContentEntryActivated(Favorite* favorite);

  void Rename(const bookmarks::BookmarkNode* favorite,
              const base::string16& title);

  void ChangeURL(const bookmarks::BookmarkNode* node, const GURL& url);
  void ChangePartnerActivationCount(const bookmarks::BookmarkNode* node,
                                    int32_t count);
  void ChangePartnerChannel(const bookmarks::BookmarkNode* node,
                            int32_t channel);
  void ChangePartnerId(const bookmarks::BookmarkNode* node, int32_t id);
  void ChangePartnerChecksum(const bookmarks::BookmarkNode* node,
                             int32_t checksum);
  void ChangePartnerGroup(const bookmarks::BookmarkNode* node, int32_t group);

  void DeleteSavedPageFile(const std::string& file);

  void RemoveNodeFavorite(Favorite* favorite);
  void RemoveNonNodeFavorite(Favorite* favorite);

 private:
  typedef std::map<int64_t, Favorite*> favorites_t;

  friend class BookmarksFolder;
  friend class FeedsFolder;
  friend class NodeFolderImpl;
  friend class RootFolderImpl;
  friend class SavedPagesFolder;
  friend class CollectionReader;

  /**
   * @name FavoritesDelegate::ThumbnailReceiver implementation
   * @{
   */
  void Finished(int64_t id, const char* title) override;
  void Error(int64_t id, bool temporary) override;
  /** @} */

  /**
   * @name FavoritesDelegate::ImportDelegate implementation
   * @{
   */
  void OpenFolder(const char* title) override;
  void OpenFolder(const char* title, int32_t partner_group_id) override;
  void CloseFolder() override;
  void ImportFavorite(const char* title,
                      const char* url,
                      const char* thumbnail_path) override;
  void ImportPartnerContent(const char* title,
                            const char* url,
                            const char* thumbnail_path,
                            int32_t pushed_partner_activation_count,
                            int32_t pushed_partner_channel,
                            int32_t pushed_partner_id,
                            int32_t pushed_partner_checksum) override;
  void ImportSavedPage(const char* title,
                       const char* file,
                       const char* url,
                       const char* thumbnail_path) override;
  /** @} */

  /**
   * @name FavoritesDelegate::PartnerContentInterface implementation
   * @{
   */

  int64_t GetRoot() override;
  bool IsPartnerContent(int64_t favorite_id) override;
  int64_t GetParent(int64_t favorite_id) override;
  int32_t GetPushActivationCount(int64_t favorite_id) override;
  int32_t GetPushId(int64_t favorite_id) override;
  int64_t FindEntryFromPushChannel(int32_t channel_id) override;
  int64_t FindFolderFromPushId(int32_t push_id) override;
  void RemovePartnerContent(int64_t favorite_id) override;
  void ResetActivationCountForAllPartnerContent() override;
  void SetPushData(int64_t favorite_id,
                   int32_t channel,
                   int32_t push_id,
                   int32_t checksum,
                   int32_t activation_count) override;
  int32_t GetChildCount(int64_t folder_id) override;
  void UpdatePushedGroupId(int64_t folder_id) override;
  int32_t* GetHeaderDataForAllPartnerContent(size_t* size) override;
  int32_t* GetUsageHeaderDataForAllPartnerContent(size_t* size) override;
  int64_t CreatePartnerContent(int64_t parent,
                               int position,
                               const char* title,
                               const char* url,
                               const int8_t* thumbnail,
                               size_t thumbnail_size,
                               int32_t partner_channel,
                               int32_t partner_id,
                               int32_t partner_checksum) override;
  int64_t CreateFolder(int position,
                       const char* title,
                       const int8_t* thumbnail,
                       size_t thumbnail_size,
                       int32_t group_id) override;
  void UpdatePartnerGroup(int64_t id,
                          const char* title,
                          const int8_t* thumbnail,
                          size_t thumbnail_size);
  void UpdatePartnerContent(int64_t id,
                            const char* title,
                            const char* url,
                            const int8_t* thumbnail,
                            size_t thumbnail_size) override;
  /** @} */

  /**
   * @name BookmarkModelObserver implementation
   * @{
   */
  void BookmarkModelLoaded(bookmarks::BookmarkModel* model,
                           bool ids_reassigned) override;
  void BookmarkModelBeingDeleted(bookmarks::BookmarkModel* model) override;
  void BookmarkNodeMoved(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* old_parent,
                         int old_index,
                         const bookmarks::BookmarkNode* new_parent,
                         int new_index) override;
  void BookmarkNodeAdded(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* parent,
                         int index) override;
  void BookmarkNodeRemoved(bookmarks::BookmarkModel* model,
                           const bookmarks::BookmarkNode* parent,
                           int old_index,
                           const bookmarks::BookmarkNode* node,
                           const std::set<GURL>& removed_urls) override;
  void OnWillChangeBookmarkNode(bookmarks::BookmarkModel* model,
                                const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeChanged(bookmarks::BookmarkModel* model,
                           const bookmarks::BookmarkNode* node) override;
  void BookmarkMetaInfoChanged(bookmarks::BookmarkModel* model,
                               const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeFaviconChanged(bookmarks::BookmarkModel* model,
                                  const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeChildrenReordered(bookmarks::BookmarkModel* model,
                                     const bookmarks::BookmarkNode* node)
                                     override;
  void BookmarkAllUserNodesRemoved(
      bookmarks::BookmarkModel* model,
      const std::set<GURL>& removed_urls) override;
  void ExtensiveBookmarkChangesBeginning(
      bookmarks::BookmarkModel* model) override;
  void ExtensiveBookmarkChangesEnded(bookmarks::BookmarkModel* model) override;
  /** @} */

  /**
   * @name SavedPagesReader::Delegate implementation
   * @{
   */
  SavedPagesFolder* CreateSavedPagesFolder() override;
  SavedPage* CreateSavedPage(const base::string16& title,
                             const GURL& url,
                             const std::string& guid,
                             const std::string& file) override;
  void DoneReadingSavedPages() override;
  /** @} */

  /**
   * @name CollectionReader::Observer implementation
   * @{
   */
  void CollectionReaderIsFinished() override;
  /** @} */

  void ImportSavedPage(const std::string& title,
                       const std::string& file,
                       const GURL& url,
                       const std::string& thumbnail_path);

  bool NodeIsPartOfFavorites(const bookmarks::BookmarkNode* node);
  bool FindFromId(int64_t id,
                  const bookmarks::BookmarkNode** parent,
                  int* index);
  bool FindFromId(int64_t id,
                  const bookmarks::BookmarkNode* root,
                  bool recursive,
                  const bookmarks::BookmarkNode** parent,
                  int* index);

  void SignalReady();
  void SignalLoaded();

  int64_t GenerateId();

  std::string GenerateThumbnailName(Favorite* favorite);

  Folder* CreateFolder(int pos,
                       const base::string16& title,
                       const std::string& guid,
                       int32_t pushed_partner_group_id);

  Favorite* CreateFavorite(Folder* parent,
                           int pos,
                           const base::string16& title,
                           const GURL& url,
                           const std::string& guid,
                           bool look_for_thumbnail);

  /**
   * Create a partner content object
   * (formerly known as pushed content in Mobile).
   *
   * @param parent The folder which the partner content should be added to
   * @param pos The position index into the folder at which position the
   *            partner content object will be positioned
   * @param title The title of the partner content object
   * @param url The url of the partner content object
   * @param guid guid of the object, may be ""
   * @param pushed_partner_activation_count The activation count since last
   *                                        USAGE ACK from the partner content
   *                                        server
   * @param pushed_partner_channel
   * @param pushed_partner_id The id used when the partner content object is
   *                          accessed from the pushed content servers
   * @param pushed_partner_checksum
   *
   * TODO(felixe): Return a PartnerContent object once such a type exists
   */
  Favorite* CreatePartnerContent(Folder* parent,
                                 int pos,
                                 const base::string16& title,
                                 const GURL& url,
                                 const std::string& guid,
                                 int32_t pushed_partner_activation_count,
                                 int32_t pushed_partner_channel,
                                 int32_t pushed_partner_id,
                                 int32_t pushed_partner_checksum,
                                 bool look_for_thumbnail);

  void CheckBookmarksFolder();

  void ImportBookmarkModel();
  void ImportFolder(Folder* dst, const bookmarks::BookmarkNode* src);

  void CheckFeeds();

  void ReadSavedPages();

  void SendDelayedThumbnailRequests();

  /**
   * @param id Id of the favorite
   * @param path thumbnail path to search for, may not be empty
   * @param ignore_if_exists if true, OnThumbnailFound will not be called
   *                         if a file exists at the given path
   */
  void LookForThumbnail(int64_t id, const std::string& path,
                        bool ignore_if_exists);
  void OnThumbnailFound(int64_t id, const std::string& path);

  /**
   * Rename the old Bream favorite thumbnail, if it exists, to the new naming
   * format: favorite_<GUID>. Will notify about the renaming through the
   * OnThumbnailFound method.
   *
   * This method should be scheduled to run on the {@link file_task_runner_}.
   *
   * @param current_path Current path to the Bream thumbnail.
   * @param id Id of the favorite
   * @param guid Guid of the favorite. May be empty
   * @param node_id Id of the favorite if it is a bookmark node, 0 otherwise
   *
   * @see OnThumbnailFound
   */
  void MoveBreamFavoriteThumbnail(const std::string& current_path,
                                  int64_t id,
                                  const std::string& guid,
                                  int64_t node_id);

  // Returns all partner content elements and *not* any of the partner folders
  std::vector<Favorite*> GetAllPartnerContent();
  // Returns all partner content folders
  std::vector<Favorite*> GetAllPartnerContentFolders();

  void SetThumbnailData(Favorite* favorite, const void* data, size_t size);

  void StoreThumbnail(int64_t id,
                      const std::string& filename,
                      const std::vector<char>& data);

  std::string GetFavoriteBaseName(const std::string& guid,
                                  int64_t node_id) const;

  void ScheduleRemoveThumbnail(Favorite* favorite);

  void FinishLoaded();

  void SetupRoot();

  void CheckPreferences(bool check_bookmark_loaded);

  void RemovePartnerContentAndThumbnail(Favorite* favorite, bool skip_parent);

  void SafeRemove(const bookmarks::BookmarkNode* parent, int index,
                  const bookmarks::BookmarkNode* node);
  void RemoveDelayed();

  int GetSpecialIndex(int32_t special);
  void SetSpecialIndex(int32_t special, int index);
  void UpdateSpecialIndexes();

  int IndexFromModel(Folder* parent, int index);
  int IndexToModel(Folder* parent, int index);
  void BeginLocalAdd();
  void EndLocalAdd();

  void RemoveAllModelChildren(Folder* root);

  void ResetLocalDevice();

  static void DisableCheck(Folder* folder);
  static void RestoreCheck(Folder* folder);

  void ForEachFolder(const base::Callback<void(Folder*)>& callback);

  void AfterExtensiveBookmarkChangesEnded();

  Folder* devices_root_;
  Folder* local_root_;
  Folder* bookmarks_folder_;
  // If saved pages are hidden, always null, otherwise the value of
  // backend_saved_pages_
  Folder* saved_pages_;
  // Always non-null if there are saved pages
  SavedPagesFolder* backend_saved_pages_;
  Folder* feeds_;

  base::string16 bookmarks_folder_title_;
  base::string16 saved_pages_title_;
  base::string16 feeds_title_;

  favorites_t favorites_;
  favorites_t model_favorites_;

  base::ObserverList<FavoritesObserver> observers_;

  int64_t last_id_;
  unsigned int icon_size_;
  unsigned int thumb_size_;

  std::string path_;

  FavoritesDelegate* delegate_;

  std::set<Favorite*> delayed_thumbnail_requests_;

  bookmarks::BookmarkModel *bookmark_model_;
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> file_task_runner_;

  bool ignore_move_;
  std::string savedpage_path_;
  int64_t import_folder_;
  std::unique_ptr<SavedPagesReader> saved_pages_reader_;
  bool loaded_;
  bool skip_thumbnail_search_;
  bool importing_from_model_;
  std::string path_suffix_;

  GURL old_node_url_;
  base::string16 old_node_title_;

  std::unique_ptr<CollectionReader> collection_reader_;
  bool import_finished_;

  bool waiting_for_saved_pages_;

  std::unique_ptr<PrefService> prefs_;
  scoped_refptr<PrefRegistry> pref_registry_;
  std::string device_name_;

  std::set<int64_t> delayed_removal_;
  bool inside_extensive_change_;
  bool after_extensive_change_posted_;

  const bookmarks::BookmarkNode* update_partner_content_node_;
  // < 0 disabled, > 0 enabled, = 0 keep-as-is
  // keep-as-is means that if there already is a folder it will be kept until
  // the folder is disabled and if there is no folder a new one won't be
  // created until the folder is enabled.
  int8_t bookmarks_folder_enabled_;
  int8_t feeds_enabled_;
  // There is no keep-as-is, saved_pages_mode must be set before loaded and
  // the default is ENABLED
  SavedPagesMode saved_pages_mode_;

  // Used by IndexToModel and IndexFromModel in the case where we add a
  // non-special item after a special one. The problem is that we add items by
  // calling the model and then reacting to BookmarksAdded but in BookmarksAdded
  // we have no idea that the item was actually supposed to be behind a
  // special folder as the index is in front of it. So, local_add_offset_
  // when active (>= 0) contains how many special folders there are in front
  // of the new item
  int32_t local_add_offset_;
};

}  // namespace mobile

#endif  // MOBILE_COMMON_FAVORITES_FAVORITES_IMPL_H_
