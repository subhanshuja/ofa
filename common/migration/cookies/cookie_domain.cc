// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/cookies/cookie_domain.h"
#include "common/migration/cookies/cookie_import_tags.h"
#include "common/migration/tools/data_stream_reader.h"
#include "net/cookies/cookie_store.h"
#include "base/logging.h"
#include "base/bind.h"

namespace opera {
namespace common {
namespace migration {

namespace {
/* With C++11, these could've been lambdas. We can't use C++11 in common
 * code due to some platforms using old compilers. */

bool ReadString(std::string* target, DataStreamReader* reader) {
  if (target)
    *target = reader->ReadString();
  return !reader->IsFailed();
}

/* Documentation sometimes says that some tag is followed by an 8bit value
 * while IRL it's a 32 bit value, with only the last bits taken. Don't be
 * surprised when tags described as indicating 8bit values use this function. */
bool ReadInt32DontStore(DataStreamReader* reader) {
  reader->ReadContentWithSize<int32_t>();
  return !reader->IsFailed();
}

bool ReadNothing(DataStreamReader* reader) {
  return !reader->IsFailed();
}

}  // namespace anonymous

CookieDomain::CookieDomain() {
  RegisterTagHandlers();
}

CookieDomain::CookieDomain(const CookieDomain&) = default;

CookieDomain::~CookieDomain() {
}

bool CookieDomain::Parse(DataStreamReader* reader) {
  if (reader->IsFailed())
    return false;
  /* Get domain record size from stream. We don't really care as we can notice
   * when the record ends by reading in TAG_COOKIE_DOMAIN_END. */
  reader->ReadSize();

  uint32_t tag = reader->ReadTag();
  while (!reader->IsFailed() && tag != TAG_COOKIE_DOMAIN_END) {
    TagHandlersMap::iterator tag_handler_iterator = tag_handlers_.find(tag);
    if (tag_handler_iterator == tag_handlers_.end()) {
      LOG(ERROR) << "Unexpected tag while importing a cookie domain: 0x"
                 << std::hex << tag;
      return false;
    }
    TagHandler handler = tag_handler_iterator->second;
    if (!handler.Run(reader)) {
      LOG(ERROR) << "Error while handling tag: 0x" << std::hex << tag;
      return false;
    }
    tag = reader->ReadTag();
  }
  return !reader->IsFailed();
}

void CookieDomain::StoreCookies(
    const std::string& parent_domain,
    const CookieCallback& on_new_cookie_callback) const {
  std::string absolute_path = domain_name_;
  if (!parent_domain.empty())
    absolute_path += std::string(".") + parent_domain;

  // Store my cookies
  for (std::vector<ImportedCookie>::const_iterator it = cookies_.begin();
       it != cookies_.end();
       ++it) {
    it->StoreSelf(absolute_path, on_new_cookie_callback);
  }

  // Store cookies of subcomponents, recursively
  for (std::vector<PathComponent>::const_iterator it = path_components_.begin();
       it != path_components_.end();
       ++it) {
    it->StoreCookies(absolute_path, on_new_cookie_callback);
  }

  // Store cookies of subdomains, recursively
  for (std::vector<CookieDomain>::const_iterator it = subdomains_.begin();
       it != subdomains_.end();
       ++it) {
    it->StoreCookies(absolute_path, on_new_cookie_callback);
  }
}

void CookieDomain::RegisterTagHandlers() {
  tag_handlers_[TAG_COOKIE_DOMAIN_ILLPATH] =
      base::Bind(&ReadInt32DontStore);
  tag_handlers_[TAG_COOKIE_DOMAIN_NAME] =
      base::Bind(&ReadString, &domain_name_);
  tag_handlers_[TAG_COOKIE_SERVER_ACCEPT_FLAG] =
      base::Bind(&ReadInt32DontStore);
  tag_handlers_[TAG_COOKIE_SERVER_ACCEPT_THIRDPARTY] =
      base::Bind(&ReadInt32DontStore);
  tag_handlers_[TAG_COOKIE_DELETE_COOKIES_ON_EXIT] =
      base::Bind(&ReadInt32DontStore);
  tag_handlers_[TAG_COOKIE_PATH_ENTRY] =
      base::Bind(&CookieDomain::ParsePath, base::Unretained(this));
  tag_handlers_[TAG_COOKIE_ENTRY] =
      base::Bind(&CookieDomain::ParseCookie, base::Unretained(this));
  tag_handlers_[TAG_COOKIE_DOMAIN_ENTRY] =
      base::Bind(&CookieDomain::ParseSubdomain, base::Unretained(this));
  tag_handlers_[TAG_COOKIE_PATH_END] =
      base::Bind(&ReadNothing);
}

bool CookieDomain::ParseCookie(DataStreamReader* reader) {
  ImportedCookie cookie;
  if (!cookie.Parse(reader)) {
    LOG(ERROR) << "Error while parsing cookie";
    return false;
  }
  cookies_.push_back(cookie);
  return true;
}

bool CookieDomain::ParsePath(DataStreamReader* reader) {
  PathComponent component;
  if (!component.Parse(reader)) {
    LOG(ERROR) << "Error while parsing path component";
    return false;
  }
  path_components_.push_back(component);
  return true;
}

bool CookieDomain::ParseSubdomain(DataStreamReader* reader) {
  CookieDomain subdomain;
  if (!subdomain.Parse(reader)) {
    LOG(ERROR) << "Error while parsing subdomain";
    return false;
  }
  subdomains_.push_back(subdomain);
  return true;
}

}  // namespace migration
}  // namespace common
}  // namespace opera
