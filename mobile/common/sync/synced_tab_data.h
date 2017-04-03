// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_SYNC_SYNCED_TAB_DATA_H_
#define MOBILE_COMMON_SYNC_SYNCED_TAB_DATA_H_

#ifdef OS_IOS
#include <string>
#endif

#include "base/strings/string16.h"
#include "url/gurl.h"

namespace mobile {

struct SyncedTabData {
#ifdef OS_IOS
  typedef std::string id_type;
#else
  typedef int id_type;
#endif
  id_type id;
  base::string16 title;
  GURL visible_url;
  GURL url;
  GURL original_request_url;

  bool operator==(const SyncedTabData& data) const {
    return id == data.id && title == data.title &&
        visible_url == data.visible_url &&
        url == data.url &&
        original_request_url == data.original_request_url;
  }
};

}  // namespace mobile

#endif  // MOBILE_COMMON_SYNC_SYNCED_TAB_DATA_H_
