// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_FAVORITES_FAVORITE_DATA_H_
#define COMMON_FAVORITES_FAVORITE_DATA_H_

#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"
#include "url/gurl.h"

namespace base {
class Pickle;
class PickleIterator;
}  // namespace base

namespace opera {

typedef std::vector<base::string16> FavoriteKeywords;

enum FavoriteExtensionState {
  /// No extension associated with this Favorite (yet).
  STATE_EXTENSION_MISSING,
  /// An extension is being installed.
  STATE_EXTENSION_INSTALLING,
  /// Extension installation failed. This favorite is used to display error.
  STATE_EXTENSION_INSTALL_FAILED,
  /// An extension is associated with this Fabvorite.
  STATE_EXTENSION_PRESENT
};

/**
 * The data held by a Favorite instance.
 */
struct FavoriteData {
  FavoriteData();
  ~FavoriteData();

  void Serialize(base::Pickle* pickle) const;
  void Deserialize(const base::Pickle& pickle);

  /**
   * Deserialize an old version of the favorite data that lacked the version
   * number. Should only be used by the code migrating favorites into a new and
   * versioned scheme.
   */
  bool DeserializeVersion0(base::PickleIterator* iter);

  // 1 - Introduce version numbers into the serialized data blob
  static const int kCurrentVersionNumber = 1;

  base::string16 title;
  GURL url;

  /** Identifies the partner content stored in this FavoriteData. */
  std::string partner_id;

  /**
   * Whether to use the redirection server or not.
   *
   * Only meaningful for partner content.  If the redirection server is used,
   * Favorite::navigate_url() returns a redirect URL.
   */
  bool redirect;

  FavoriteKeywords custom_keywords;

  std::string extension;

  std::string extension_id;

  FavoriteExtensionState extension_state;

  /**
   * The title that can be changed by a running Speed Dial extension
   * associated with a Favorite.
   *
   * It's initialized with the value of #title so each time the extension
   * starts it has the original value.
   */
  base::string16 live_title;

  /**
   * The URL that can be changed by a running Speed Dial Extension
   * associated with a Favorite.
   *
   * It's initialized with the value of #url so each time the extension
   * starts it has the original value.
   */
  GURL live_url;

  /**
   * Flag for magic "virtual" folders which serve as placehold markers for
   * abstractions on top of the favorite api:s
   */
  bool is_placeholder_folder;

  /**
   * The activation count (number of times the favorite has been clicked) since
   * last USAGE ACK from the partner content servers.
   */
  int pushed_partner_activation_count;

  /**
   * The partner channel that the favorite belongs to.
   */
  int pushed_partner_channel;

  /**
   * The id which this favorite is referenced through when applying changes
   * from the partner content servers.
   */
  int pushed_partner_id;

  /**
   * The id of the favorite folder used for reference when applying changes
   * from the partner content servers.
   *
   * Only valid for folder objects.
   */
  int pushed_partner_group_id;

  /**
   * The activation count (number of times the favorite has been clicked) since
   * last USAGE ACK from the partner content servers.
   */
  int pushed_partner_checksum;

 private:
  bool DeserializeVersion1(base::PickleIterator* iter);
};


/**
 * A holder of FavoriteData.
 *
 * The purpose is to give FavoriteCollectionImpl privileged access to the data
 * of its Favorite objects, but not to other private properties of Favorite.
 */
class FavoriteDataHolder {
 public:
  const FavoriteData& data() const { return data_; }

 protected:
  explicit FavoriteDataHolder(const FavoriteData& data);

 private:
  friend class FavoriteCollectionImpl;

  FavoriteData data_;
};

}  // namespace opera

#endif  // COMMON_FAVORITES_FAVORITE_DATA_H_
