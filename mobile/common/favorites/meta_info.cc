// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/favorites/meta_info.h"

#include "base/strings/string_number_conversions.h"
#include "components/sync_bookmarks/bookmark_change_processor.h"

using bookmarks::BookmarkNode;

namespace {

const std::string kEmptyGUID = "";

const std::string kOperaMetaInfoGUID = "opera_favorite_guid";
const std::string kOperaMetaInfoPushedPartner = "opera_favorite_pushed_partner";
const std::string kOperaMetaInfoSpecialIndex = "opera_favorite_special_index";

const int kPushedPartnerActivationCount = 0;
const int kPushedPartnerGroupId = 1;
const int kPushedPartnerId = 2;
const int kPushedPartnerChannel = 3;
const int kPushedPartnerChecksum = 4;

void UpdatePushedPartnerData(BookmarkNode::MetaInfoMap& meta_info,
                             int index,
                             int32_t new_value) {
  auto i = meta_info.find(kOperaMetaInfoPushedPartner);
  if (i != meta_info.end() && !i->second.empty()) {
    std::string::size_type start = 0;
    while (index-- > 0) {
      std::string::size_type next = i->second.find(',', start);
      if (next == std::string::npos) {
        start = i->second.size() + 1;
        i->second.append(",0");
      } else {
        start = next + 1;
      }
    }
    std::string::size_type end = i->second.find(',', start);
    if (end == std::string::npos) {
      end = i->second.size();
    }
    i->second.replace(start, end - start, base::Int64ToString(new_value));
  } else {
    std::string data;
    while (index-- > 0) data.append("0,");
    data.append(base::Int64ToString(new_value));
    meta_info.insert(std::make_pair(kOperaMetaInfoPushedPartner, data));
  }
}

int GetPushedPartnerData(const BookmarkNode* node, int index) {
  if (!node) return 0;
  const BookmarkNode::MetaInfoMap* map = node->GetMetaInfoMap();
  if (!map) return 0;
  auto i = map->find(kOperaMetaInfoPushedPartner);
  if (i == map->end()) return 0;
  std::string::size_type start = 0;
  while (index-- > 0) {
    std::string::size_type next = i->second.find(',', start);
    if (next == std::string::npos) {
      return 0;
    }
    start = next + 1;
  }
  std::string::size_type end = i->second.find(',', start);
  if (end == std::string::npos) {
    end = i->second.size();
  }
  int tmp;
  if (base::StringToInt(i->second.substr(start, end - start), &tmp)) {
    return tmp;
  }
  return 0;
}

}  // namespace

namespace mobile {

MetaInfo::MetaInfo() {
}

MetaInfo::~MetaInfo() {
}

// static
void MetaInfo::SetGUID(BookmarkNode::MetaInfoMap& meta_info,
                       const std::string& guid) {
  auto i = meta_info.find(kOperaMetaInfoGUID);
  if (i != meta_info.end()) {
    if (guid.empty()) {
      meta_info.erase(i);
    } else {
      i->second = guid;
    }
  } else {
    if (!guid.empty()) {
      meta_info.insert(std::make_pair(kOperaMetaInfoGUID, guid));
    }
  }
}

// static
const std::string& MetaInfo::GetGUID(const BookmarkNode* node) {
  DCHECK(node);
  const BookmarkNode::MetaInfoMap* map = node->GetMetaInfoMap();
  if (map) {
    auto i = map->find(kOperaMetaInfoGUID);
    if (i != map->end()) {
      return i->second;
    }
  }
  return kEmptyGUID;
}

// static
void MetaInfo::SetPushedPartnerGroupId(BookmarkNode::MetaInfoMap& meta_info,
                                       int32_t pushed_parter_group_id) {
  UpdatePushedPartnerData(meta_info,
                          kPushedPartnerGroupId,
                          pushed_parter_group_id);
}

// static
int MetaInfo::GetPushedPartnerGroupId(const BookmarkNode* node) {
  return GetPushedPartnerData(node, kPushedPartnerGroupId);
}

// static
void MetaInfo::SetPushedPartnerId(BookmarkNode::MetaInfoMap& meta_info,
                                  int32_t pushed_parter_id) {
  UpdatePushedPartnerData(meta_info,
                          kPushedPartnerId,
                          pushed_parter_id);
}

// static
int MetaInfo::GetPushedPartnerId(const BookmarkNode* node) {
  return GetPushedPartnerData(node, kPushedPartnerId);
}

// static
void MetaInfo::SetPushedPartnerChannel(BookmarkNode::MetaInfoMap& meta_info,
                                       int32_t pushed_parter_channel) {
  UpdatePushedPartnerData(meta_info,
                          kPushedPartnerChannel,
                          pushed_parter_channel);
}

// static
int MetaInfo::GetPushedPartnerChannel(const BookmarkNode* node) {
  return GetPushedPartnerData(node, kPushedPartnerChannel);
}

// static
void MetaInfo::SetPushedPartnerActivationCount(
    BookmarkNode::MetaInfoMap& meta_info,
    int32_t pushed_parter_activation_count) {
  UpdatePushedPartnerData(meta_info,
                          kPushedPartnerActivationCount,
                          pushed_parter_activation_count);
}

// static
int MetaInfo::GetPushedPartnerActivationCount(const BookmarkNode* node) {
  return GetPushedPartnerData(node, kPushedPartnerActivationCount);
}

// static
void MetaInfo::SetPushedPartnerChecksum(BookmarkNode::MetaInfoMap& meta_info,
                                        int32_t pushed_parter_checksum) {
  UpdatePushedPartnerData(meta_info,
                          kPushedPartnerChecksum,
                          pushed_parter_checksum);
}

// static
int MetaInfo::GetPushedPartnerChecksum(const BookmarkNode* node) {
  return GetPushedPartnerData(node, kPushedPartnerChecksum);
}

// static
void MetaInfo::SetSpecialIndex(BookmarkNode::MetaInfoMap& meta_info,
                               int32_t special, int index) {
  auto i = meta_info.find(kOperaMetaInfoSpecialIndex);
  std::string data;
  if (i != meta_info.end()) {
    data = i->second;
  }

  const std::string key = "," + base::Int64ToString(special) + ":";
  size_t pos = data.find(key);
  if (pos != std::string::npos) {
    size_t end = data.find(',', pos + key.size());
    if (end == std::string::npos) end = data.size();
    if (index < 0) {
      data.erase(pos, end - pos);
    } else {
      pos += key.size();
      data.replace(pos, end - pos, base::Int64ToString(index));
    }
  } else {
    if (index < 0) {
      return;
    } else {
      data.append(key).append(base::Int64ToString(index));
    }
  }

  if (data.empty()) {
    if (i != meta_info.end()) {
      meta_info.erase(i);
    }
  } else {
    if (i != meta_info.end()) {
      i->second = data;
    } else {
      meta_info.insert(std::make_pair(kOperaMetaInfoSpecialIndex, data));
    }
  }
}

// static
int MetaInfo::GetSpecialIndex(const BookmarkNode* node, int32_t special) {
  DCHECK(node);
  const BookmarkNode::MetaInfoMap* map = node->GetMetaInfoMap();
  if (map) {
    auto i = map->find(kOperaMetaInfoSpecialIndex);
    if (i != map->end()) {
      const std::string key = "," + base::Int64ToString(special) + ":";
      size_t pos = i->second.find(key);
      if (pos != std::string::npos) {
        pos += key.size();
        size_t end = i->second.find(',', pos);
        if (end == std::string::npos) end = i->second.size();
        int64_t ret;
        if (base::StringToInt64(i->second.substr(pos, end - pos), &ret)) {
          return static_cast<int>(ret);
        }
      }
    }
  }
  return -1;
}

// static
void MetaInfo::Register() {
  sync_bookmarks::BookmarkChangeProcessor::RegisterLocalOnlyMetaInfo(
      kOperaMetaInfoPushedPartner);
  sync_bookmarks::BookmarkChangeProcessor::RegisterLocalOnlyMetaInfo(
      kOperaMetaInfoSpecialIndex);
}

}  // namespace mobile
