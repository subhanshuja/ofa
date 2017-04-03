// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_META_INFO_H_
#define MOBILE_COMMON_FAVORITES_META_INFO_H_

#include <string>

#include "components/bookmarks/browser/bookmark_node.h"

namespace mobile {

class MetaInfo {
 public:
  static void SetGUID(bookmarks::BookmarkNode::MetaInfoMap& meta_info,
                      const std::string& guid);
  static const std::string& GetGUID(const bookmarks::BookmarkNode* node);

  static void SetPushedPartnerGroupId(
      bookmarks::BookmarkNode::MetaInfoMap& meta_info,
      int32_t pushed_parter_group_id);
  static int32_t GetPushedPartnerGroupId(const bookmarks::BookmarkNode* node);

  static void SetPushedPartnerId(
      bookmarks::BookmarkNode::MetaInfoMap& meta_info,
      int32_t pushed_parter_id);
  static int32_t GetPushedPartnerId(const bookmarks::BookmarkNode* node);

  static void SetPushedPartnerChannel(
      bookmarks::BookmarkNode::MetaInfoMap& meta_info,
      int32_t pushed_parter_channel);
  static int32_t GetPushedPartnerChannel(const bookmarks::BookmarkNode* node);

  static void SetPushedPartnerActivationCount(
      bookmarks::BookmarkNode::MetaInfoMap& meta_info,
      int32_t pushed_parter_activation_count);
  static int GetPushedPartnerActivationCount(
      const bookmarks::BookmarkNode* node);

  static void SetPushedPartnerChecksum(
      bookmarks::BookmarkNode::MetaInfoMap& meta_info,
      int32_t pushed_parter_checksum);
  static int32_t GetPushedPartnerChecksum(const bookmarks::BookmarkNode* node);

  static void SetSpecialIndex(bookmarks::BookmarkNode::MetaInfoMap& meta_info,
                              int32_t special,
                              int index);
  static int GetSpecialIndex(const bookmarks::BookmarkNode* node,
                             int32_t special);

  static void Register();

 private:
  MetaInfo();
  ~MetaInfo();
};

}  // namespace mobile

#endif  // MOBILE_COMMON_FAVORITES_META_INFO_H_
