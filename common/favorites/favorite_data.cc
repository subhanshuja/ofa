// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/favorites/favorite_data.h"

#include "base/pickle.h"

namespace opera {

FavoriteData::FavoriteData()
    : redirect(false),
      is_placeholder_folder(false),
      pushed_partner_activation_count(0),
      pushed_partner_channel(0),
      pushed_partner_id(0),
      pushed_partner_group_id(0),
      pushed_partner_checksum(0) {
}

FavoriteData::~FavoriteData() {
}

void FavoriteData::Serialize(base::Pickle* data) const {
  data->WriteInt(kCurrentVersionNumber);
  data->WriteInt(custom_keywords.size());
  for (FavoriteKeywords::const_iterator it = custom_keywords.begin();
       it != custom_keywords.end();
       ++it)
    data->WriteString16(*it);

  data->WriteBool(is_placeholder_folder);
  data->WriteBool(redirect);
  data->WriteString(partner_id);

  data->WriteInt(pushed_partner_activation_count);
  data->WriteInt(pushed_partner_channel);
  data->WriteInt(pushed_partner_id);
  data->WriteInt(pushed_partner_group_id);
  data->WriteInt(pushed_partner_checksum);
}

void FavoriteData::Deserialize(const base::Pickle& data) {
  base::PickleIterator iter(data);

  int version;
  if (!iter.ReadInt(&version))
    return;

  DeserializeVersion1(&iter);
}

bool FavoriteData::DeserializeVersion0(base::PickleIterator* iter) {
  // Version 0 is the same as version 1, except is_placeholder_folder used to
  // be data_owned_by_user, and basically mean the same thing but inverted.
  //
  // is_placeholder_folder is assumed to be false for all old version 0
  // favorites
  if (DeserializeVersion1(iter)) {
    is_placeholder_folder = false;
    return true;
  }

  return false;
}

bool FavoriteData::DeserializeVersion1(base::PickleIterator* iter) {
  int num_keywords = 0;
  if (!iter->ReadInt(&num_keywords))
    return false;

  for (int i = 0; i < num_keywords; i++) {
    base::string16 keyword;
    if (!iter->ReadString16(&keyword))
      return false;
    custom_keywords.push_back(keyword);
  }

  if (!iter->ReadBool(&is_placeholder_folder))
    return false;
  if (!iter->ReadBool(&redirect))
    return false;
  if (!iter->ReadString(&partner_id))
    return false;

  if (!iter->ReadInt(&pushed_partner_activation_count))
    return false;
  if (!iter->ReadInt(&pushed_partner_channel))
    return false;
  if (!iter->ReadInt(&pushed_partner_id))
    return false;
  if (!iter->ReadInt(&pushed_partner_group_id))
    return false;
  if (!iter->ReadInt(&pushed_partner_checksum))
    return false;

  return true;
}

FavoriteDataHolder::FavoriteDataHolder(const FavoriteData& data)
    : data_(data) {
}

}  // namespace opera
