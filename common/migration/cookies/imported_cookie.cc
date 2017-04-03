// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/cookies/imported_cookie.h"

#include <iomanip>
#include <sstream>

#include "base/bind.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "net/cookies/cookie_store.h"
#include "net/cookies/parsed_cookie.h"
#include "url/gurl.h"

#include "common/migration/cookies/cookie_import_tags.h"
#include "common/migration/tools/data_stream_reader.h"

namespace opera {
namespace common {
namespace migration {

namespace {
/* With C++11, these could've been lambdas. We can't use C++11 in common
 * code due to some platforms using old compilers. */

template<typename T>
void Read(T* target, DataStreamReader* reader) {
  T obj = reader->ReadContentWithSize<T>();
  if (target)
    *target = obj;
}

void ReadString(std::string* target, DataStreamReader* reader) {
  std::string obj = reader->ReadString();
  if (target)
    *target = obj;
}

void ReadInt8(int8_t* target, DataStreamReader* reader) {
  Read(target, reader);
}

void ReadInt64(int64_t* target, DataStreamReader* reader) {
  Read(target, reader);
}

void SetTrue(bool* target, DataStreamReader* /*reader*/) {
  *target = true;
}

void DoNothing(DataStreamReader* /*reader*/) {}
}  // namespace anonymous

ImportedCookie::ImportedCookie()
    : expiry_date_(0),
      secure_(false),
      http_only_(false),
      only_server_(false),
      version_number_(0) {
  RegisterTagHandlers();
}

ImportedCookie::ImportedCookie(const ImportedCookie&) = default;

ImportedCookie::~ImportedCookie() {
}

bool ImportedCookie::Parse(DataStreamReader* reader) {
  if (reader->IsFailed())
    return false;
  size_t cookie_record_size  = reader->ReadSize();
  size_t record_end_position = reader->GetCurrentStreamPosition() +
                               cookie_record_size;
  while (!reader->IsEof() &&
         !reader->IsFailed() &&
         reader->GetCurrentStreamPosition() < record_end_position) {
    uint32_t tag = reader->ReadTag();
    TagHandlersMap::iterator tag_handler_iterator = tag_handlers_.find(tag);
    if (tag_handler_iterator == tag_handlers_.end()) {
      LOG(ERROR) << "Unexpected tag while importing a cookie: 0x"
                 << std::hex << tag;
      return false;
    }
    TagHandler handler = tag_handler_iterator->second;
    handler.Run(reader);
  }

  return !reader->IsFailed()
      && !name_.empty();
}

std::string ImportedCookie::CreateCookieLine() const {
  std::ostringstream cookie_line;
  cookie_line << name_;
  if (!value_.empty())
    cookie_line << "=" << value_;
  if (expiry_date_) {
    // Clamp expiry_date_ to <0, kuint32max>
    int64_t safe_expiry_date = std::max(
        std::min(expiry_date_,
                 static_cast<int64_t>(std::numeric_limits<uint32_t>::max())),
        static_cast<int64_t>(0));
    base::Time expiry = base::Time::UnixEpoch();
    expiry += base::TimeDelta::FromSeconds(safe_expiry_date);
    base::Time::Exploded exploaded_expiry;
    expiry.UTCExplode(&exploaded_expiry);
    struct tm timeinfo;
    timeinfo.tm_hour = exploaded_expiry.hour;
    timeinfo.tm_min = exploaded_expiry.minute;
    timeinfo.tm_mon = exploaded_expiry.month - 1;
    timeinfo.tm_sec = exploaded_expiry.second;
    timeinfo.tm_wday = exploaded_expiry.day_of_week;
    timeinfo.tm_mday = exploaded_expiry.day_of_month;
    timeinfo.tm_year = exploaded_expiry.year - 1900;
    timeinfo.tm_isdst = -1;
    char buffer[80];
    std::strftime(buffer, 80, "%a, %d-%b-%Y %H:%M:%S GMT", &timeinfo);
    cookie_line << "; Expires=" << buffer;
  }
  if (!comment_.empty())
    cookie_line << "; Comment=" << comment_;
  if (!comment_url_.empty())
    cookie_line << "; CommentURL=" << comment_url_;
  if (!domain_.empty())
    cookie_line << "; Domain=" << domain_;
  if (!path_.empty())
    cookie_line << "; Path=" << path_;
  if (!port_limitations_.empty())
    cookie_line << "; Port=" << port_limitations_;
  if (version_number_)
    cookie_line << "; Version=" << static_cast<int32_t>(version_number_);
  if (secure_)
    cookie_line << "; Secure";
  if (http_only_)
    cookie_line << "; HttpOnly";
  return cookie_line.str();
}

void ImportedCookie::RegisterTagHandlers() {
  /* These lambdas will be called whenever a specific tag is encountered during
   * parsing. Each handler should Read* stuff so that the next thing the reader
   * will Read is the next tag (or end of cookie).
   */
  tag_handlers_[TAG_COOKIE_COMMENT] = base::Bind(ReadString, &comment_);
  tag_handlers_[TAG_COOKIE_COMMENT_URL] = base::Bind(ReadString, &comment_url_);
  tag_handlers_[TAG_COOKIE_EXPIRES] = base::Bind(ReadInt64, &expiry_date_);
  tag_handlers_[TAG_COOKIE_HTTP_ONLY] = base::Bind(SetTrue, &http_only_);
  tag_handlers_[TAG_COOKIE_NAME] = base::Bind(ReadString, &name_);
  tag_handlers_[TAG_COOKIE_SECURE] = base::Bind(SetTrue, &secure_);
  tag_handlers_[TAG_COOKIE_VALUE] = base::Bind(ReadString, &value_);
  tag_handlers_[TAG_COOKIE_VERSION] = base::Bind(ReadInt8, &version_number_);
  tag_handlers_[TAG_COOKIE_RECVD_DOMAIN] = base::Bind(ReadString, &domain_);
  tag_handlers_[TAG_COOKIE_RECVD_PATH] = base::Bind(ReadString, &path_);
  tag_handlers_[TAG_COOKIE_PORT] = base::Bind(ReadString, &port_limitations_);
  tag_handlers_[TAG_COOKIE_ONLY_SERVER] = base::Bind(SetTrue, &only_server_);

  /* The following do nothing. These tags are not imported as they don't map to
   * a cookie line (which Chromium uses to set cookies) and are thus considered
   * specific to Presto Opera.
   * These are MSB flags.
   */
  TagHandler do_nothing = base::Bind(&DoNothing);
  tag_handlers_[TAG_COOKIE_PROTECTED] = do_nothing;
  tag_handlers_[TAG_COOKIE_NOT_FOR_PREFIX] = do_nothing;
  tag_handlers_[TAG_COOKIE_HAVE_PASSWORD] = do_nothing;
  tag_handlers_[TAG_COOKIE_HAVE_AUTHENTICATION] = do_nothing;
  tag_handlers_[TAG_COOKIE_ACCEPTED_AS_THIRDPARTY] = do_nothing;
  tag_handlers_[TAG_COOKIE_ASSIGNED] = do_nothing;

  /* We don't care about these tags either but they're followed by data, as
   * opposed to MSB flags. We need to take that data off the reader so the next
   * thing it parses is a tag of the following item.
   */
  tag_handlers_[TAG_COOKIE_SERVER_ACCEPT_THIRDPARTY] =
      base::Bind(ReadInt8, static_cast<int8_t*>(NULL));
  tag_handlers_[TAG_COOKIE_DELETE_COOKIES_ON_EXIT] =
      base::Bind(ReadInt8, static_cast<int8_t*>(NULL));
  tag_handlers_[TAG_COOKIE_LAST_SYNC] =
      base::Bind(ReadInt64, static_cast<int64_t*>(NULL));
  tag_handlers_[TAG_COOKIE_LAST_USED] =
      base::Bind(ReadInt64, static_cast<int64_t*>(NULL));
}

void ImportedCookie::StoreSelf(
    const std::string &url,
    const CookieCallback& on_new_cookie_callback) const {
  if (on_new_cookie_callback.is_null()) {
    LOG(ERROR) << "No cookie callback, will not store cookie " << name_;
    return;
  }
  std::string protocol = secure_ ? "https://" : "http://";
  GURL cookie_url(protocol + url);
  std::string cookie_line = CreateCookieLine();
  if (!only_server_ && domain_.empty()) {
    // This cookie is supposed to be accessible from subdomains of the url
    cookie_line += "; Domain=." + url;
  }
  VLOG(1) << "Storing cookie: URL " << protocol + url
          << " Line: " << cookie_line;
  on_new_cookie_callback.Run(cookie_url, cookie_line);
}

}  // namespace migration
}  // namespace common
}  // namespace opera
