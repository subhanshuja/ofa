// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/cookies/path_component.h"
#include "common/migration/tools/data_stream_reader.h"
#include "common/migration/cookies/cookie_import_tags.h"
#include "net/cookies/cookie_store.h"
#include "base/logging.h"

namespace opera {
namespace common {
namespace migration {

PathComponent::PathComponent() {
}

PathComponent::PathComponent(const PathComponent&) = default;

PathComponent::~PathComponent() {
}

bool PathComponent::Parse(DataStreamReader* reader) {
  /* Take size of the path component from stream. Don't care about the value,
   * there's a terminating tag we can search for. */
  reader->ReadSize();

  uint32_t tag = reader->ReadTag();
  while (!reader->IsEof() &&
         !reader->IsFailed() &&
         tag != TAG_COOKIE_PATH_END) {
    switch (tag) {
      case TAG_COOKIE_PATH_NAME: {
          // Name
          name_ = reader->ReadString();
          break;
        }
      case TAG_COOKIE_ENTRY: {
          // Cookie
          ImportedCookie cookie;
          if (!cookie.Parse(reader)) {
            LOG(ERROR) << "Error while parsing cookie";
            return false;
          }
          cookies_.push_back(cookie);
          break;
        }
      case TAG_COOKIE_PATH_ENTRY: {
          // Subcomponent
          PathComponent subcomponent;
          if (!subcomponent.Parse(reader)) {
            LOG(ERROR) << "Error while parsing subpath";
            return false;
          }
          subcomponents_.push_back(subcomponent);
          break;
        }
      default:
        LOG(ERROR) << "Unexpected tag while importing a path component: 0x"
                   << std::hex << tag;
        return false;
    }
    tag = reader->ReadTag();
  }

  return !name_.empty();  // If name wasn't parsed, path component is broken.
}

void PathComponent::StoreCookies(
    const std::string& parent_path,
    const CookieCallback& on_new_cookie_callback) const {
  std::string absolute_path = parent_path + std::string("/") + name_;

  // Store my cookies
  for (std::vector<ImportedCookie>::const_iterator it = cookies_.begin();
       it != cookies_.end();
       ++it) {
    it->StoreSelf(absolute_path, on_new_cookie_callback);
  }

  // Store cookies of subcomponents, recursively
  for (std::vector<PathComponent>::const_iterator it = subcomponents_.begin();
       it != subcomponents_.end();
       ++it) {
    it->StoreCookies(absolute_path, on_new_cookie_callback);
  }
}

}  // namespace migration
}  // namespace common
}  // namespace opera
