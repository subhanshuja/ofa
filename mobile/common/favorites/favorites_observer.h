// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_FAVORITES_OBSERVER_H_
#define MOBILE_COMMON_FAVORITES_FAVORITES_OBSERVER_H_

#include <stdint.h>

namespace mobile {

class FavoritesObserver {
 public:
  virtual ~FavoritesObserver() {}

  FavoritesObserver() {}

  /**
   * Called when Favorites is ready for use. You can now read and create
   * favorites and folders.
   */
  virtual void OnReady() = 0;

  /**
   * Called after OnReady() when all stored favorites and folders have been
   * read.
   */
  virtual void OnLoaded() = 0;

  /**
   * Called whenever a new favorite is added.
   * Even if the addition of this new favorite causes other favorites in the
   * same parent to move no moved events will be generated.
   * @param id added favorite id
   * @param parent parent favorite id
   * @param pos position in parent
   */
  virtual void OnAdded(int64_t id, int64_t parent, int pos) = 0;

  /**
   * Called whenever a favorite is removed.
   * Even if the removal of this favorite causes other favorites in the
   * same parent to move no moved events will be generated.
   * @param id removed favorite id, no longer valid
   * @param old_parent parent favorite id
   * @param old_pos position in parent before being removed
   * @param recursive true if this is caused by a recursive remove, ie. if
   *                  this is a child being removed because the parent is.
   */
  virtual void OnRemoved(int64_t id,
                         int64_t old_parent,
                         int old_pos,
                         bool recursive) = 0;

  /**
   * Called whenever a favorite is moved.
   * @param id moved favorite id
   * @param old_parent id of parent before the move
   * @param old_pos index in old_parent before the move
   * @param new_parent id of parent after the move
   * @param new_pos index in new_parent after the move
   */
  virtual void OnMoved(int64_t id,
                       int64_t old_parent,
                       int old_pos,
                       int64_t new_parent,
                       int new_pos) = 0;

  /**
   * Flags for what has changed when OnChanged is called.
   * All flags may be combined.
   * Meta data is collection for everything that isn't otherwise listed,
   * currently that means partner content information.
   */
  enum OnChangedMask {
    TITLE_CHANGED     = 0x1,
    URL_CHANGED       = 0x2,
    THUMBNAIL_CHANGED = 0x4,
    METADATA_CHANGED  = 0x8,
    FILE_CHANGED      = 0x16,
  };

  /**
   * Called whenever the data in a favorite is changed.
   * @param id changed favorite id
   * @param parent parent favorite id
   * @param changed mask of what changed
   */
  virtual void OnChanged(int64_t id, int64_t parent, unsigned int changed) = 0;
};

}  // namespace mobile

#endif  // MOBILE_COMMON_FAVORITES_FAVORITES_OBSERVER_H_
