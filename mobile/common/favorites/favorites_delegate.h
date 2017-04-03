// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_FAVORITES_DELEGATE_H_
#define MOBILE_COMMON_FAVORITES_FAVORITES_DELEGATE_H_

// This file may not use STL

#include <stdint.h>

#include "mobile/common/favorites/favorites_export.h"

namespace mobile {

class FavoritesDelegate {
 public:
  class ThumbnailReceiver {
   public:
    virtual ~ThumbnailReceiver() {}

    virtual void Finished(int64_t id, const char* title) = 0;
    virtual void Error(int64_t id, bool temporary) = 0;

   protected:
    ThumbnailReceiver() {}
  };

  /**
   * ImportDelegate is the interface by which Bream's favorite and bookmark
   * objects are imported into the new datastores.
   *
   * The import happens in different ways and for different reasons for the two
   * datatypes. Bookmarks are imported one-by-one as they're converted to
   * Favorites by the user. Favorites in the Bream datastore on the other hand
   * are imported in a one-shot way to migrate all of them to the new
   * datastore.
   *
   * Objects are to be imported into the root folder, unless made after a call
   * to OpenFolder and before a call to CloseFolder, in which case they should
   * be imported into the folder created by the OpenFolder call.
   */
  class ImportDelegate {
   public:
    virtual ~ImportDelegate() {}
    /**
     * Create a new folder in the root with the given name. All following calls
     * to ImportFavorite, until a corresponding call to CloseFolder is made, are
     * to be placed in the newly created folder. This method will not be called
     * for the root folder itself.
     *
     * @param title The title of the folder
     */
    virtual void OpenFolder(const char* title) = 0;

    virtual void OpenFolder(const char* title, int32_t partner_group_id) = 0;

    /**
     * Close the folder created by OpenFolder. Objects imported through
     * ImportFavorite after calling this method should be placed in the root
     * folder (unless a new folder is created by OpenFolder).
     */
    virtual void CloseFolder() = 0;

    /**
     * Import the favorite with the given parameters.
     *
     * Don't EVER call into Bream in this delagate callback! The pointers are
     * not stable through Bream calls.
     *
     * @param title The title of the favorite. May be null.
     * @param url The url of the favorite. May be null.
     * @param thumbnail_path The full path to the favorite of the favorite. May
     *                       be null.
     */
    virtual void ImportFavorite(const char* title,
                                const char* url,
                                const char* thumbnail_path) = 0;

    virtual void ImportPartnerContent(const char* title,
                                      const char* url,
                                      const char* thumbnail_path,
                                      int32_t pushed_partner_activation_count,
                                      int32_t pushed_partner_channel,
                                      int32_t pushed_partner_id,
                                      int32_t pushed_partner_checksum) = 0;

    /**
     * Import the saved page with the given parameters.
     *
     * Don't EVER call into Bream in this delagate callback! The pointers are
     * not stable through Bream calls.
     *
     * @param file The file of the saved page.
     * @param thumbnail_path The full path to the favorite of the favorite. May
     *                       be null.
     */
    virtual void ImportSavedPage(const char* title,
                                 const char* url,
                                 const char* file,
                                 const char* thumbnail_path) = 0;

   protected:
    ImportDelegate() {}
  };

  class PartnerContentInterface {
   public:
    // Getting initial objects to work with
    virtual int64_t GetRoot() = 0;

    // Querying favorite
    virtual bool IsPartnerContent(int64_t favorite_id) = 0;
    virtual int64_t GetParent(int64_t favorite_id) = 0;
    virtual int32_t GetPushActivationCount(int64_t favorite_id) = 0;
    virtual int32_t GetPushId(int64_t favorite_id) = 0;
    // FindEntryFromPushChannel should not include folders
    virtual int64_t FindEntryFromPushChannel(int32_t channel_id) = 0;
    virtual int64_t FindFolderFromPushId(int32_t push_id) = 0;

    // Modifying favorite
    virtual void RemovePartnerContent(int64_t favorite_id) = 0;
    virtual void ResetActivationCountForAllPartnerContent() = 0;
    virtual void SetPushData(int64_t favorite_id,
                             int32_t channel,
                             int32_t push_id,
                             int32_t checksum,
                             int32_t activation_count) = 0;

    // Querying folder
    virtual int32_t GetChildCount(int64_t folder_id) = 0;
    virtual void UpdatePushedGroupId(int64_t folder_id) = 0;

    // Getting header data
    // Return a list of (channel, pushId, checksum) tupels, may return NULL
    // if empty. Return memory must be allocated using new[] and is now owned
    // by the receiver.
    virtual int32_t* GetHeaderDataForAllPartnerContent(size_t* size) = 0;

    // Return a list of (pushId, activationCount) tupels, may return NULL
    // if empty. Return memory must be allocated using new[] and is now owned
    // by the receiver.
    virtual int32_t* GetUsageHeaderDataForAllPartnerContent(size_t* size) = 0;

    virtual int64_t CreatePartnerContent(int64_t parent,
                                         int position,
                                         const char* title,
                                         const char* url,
                                         const int8_t* thumbnail,
                                         size_t thumbnail_size,
                                         int32_t partner_channel,
                                         int32_t partner_id,
                                         int32_t partner_checksum) = 0;

    virtual int64_t CreateFolder(int position,
                                 const char* title,
                                 const int8_t* thumbnail,
                                 size_t thumbnail_size,
                                 int32_t partner_group_id) = 0;

    virtual void UpdatePartnerGroup(int64_t id,
                                    const char* title,
                                    const int8_t* thumbnail,
                                    size_t thumbnail_size) = 0;

    virtual void UpdatePartnerContent(int64_t id,
                                      const char* title,
                                      const char* url,
                                      const int8_t* thumbnail,
                                      size_t thumbnail_size) = 0;
  };

  virtual ~FavoritesDelegate() {}

  virtual void SetRequestGraphicsSizes(int icon_size, int thumb_size) = 0;
  /* The receiver object must be alive until either Finished or Error has been
   * called on it or CancelThumbnailRequest is called. */
  virtual void RequestThumbnail(int64_t id,
                                const char* url,
                                const char* path,
                                bool fetchTitle,
                                ThumbnailReceiver* receiver) = 0;
  virtual void CancelThumbnailRequest(int64_t id) = 0;

  virtual void SetPartnerContentInterface(
      PartnerContentInterface* interface) = 0;
  virtual void OnPartnerContentActivated() = 0;
  virtual void OnPartnerContentRemoved(int32_t partner_id) = 0;
  virtual void OnPartnerContentEdited(int32_t partner_id) = 0;

  /**
   * Return suffix added to created thumbnails. May be NULL (equal to "")
   */
  virtual const char* GetPathSuffix() const = 0;

 protected:
  FavoritesDelegate() {}
};

typedef void (*SetFavoritesDelegateSignature)(FavoritesDelegate* delegate);

}  // namespace mobile

extern "C" {
FAVORITES_EXPORT void SetFavoritesDelegate(mobile::FavoritesDelegate* delegate);
}

#endif  // MOBILE_COMMON_FAVORITES_FAVORITES_DELEGATE_H_
