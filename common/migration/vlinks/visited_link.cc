// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/vlinks/visited_link.h"
#include "common/migration/vlinks/vlink4_tags.h"
#include "base/logging.h"
#include "common/migration/tools/data_stream_reader.h"

namespace opera {
namespace common {
namespace migration {

VisitedLink::VisitedLink()
    : last_visited_(0) {
}

VisitedLink::VisitedLink(const VisitedLink&) = default;

VisitedLink::~VisitedLink() {
}

bool VisitedLink::Parse(DataStreamReader* reader) {
  if (reader->IsFailed())
    return false;

  /* Assume previous parsing extracted TAG_VISITED_FILE_ENTRY from the reader
   * (and this is how we knew to call VisitedLink::Parse()). So the next thing
   * on the stream is the record size. */
  size_t cookie_record_size  = reader->ReadSize();
  size_t record_end_position = reader->GetCurrentStreamPosition() +
                               cookie_record_size;
  while (!reader->IsEof() &&
         !reader->IsFailed() &&
         reader->GetCurrentStreamPosition() < record_end_position) {
    uint32_t tag = reader->ReadTag();
    switch (tag) {
      case TAG_LAST_VISITED:
        last_visited_ = reader->ReadContentWithSize<uint32_t>();
        break;
      case TAG_URL_NAME:
        url_ = GURL(reader->ReadString());
        break;
      case TAG_RELATIVE_ENTRY:
        if (!ParseRelativeLink(reader))
          return false;
        break;
      default:
        LOG(ERROR) << "Unexpected tag received: 0x" << std::hex << tag;
        return false;
    }
  }
  return url_.is_valid() && last_visited_ && !reader->IsFailed();
}

bool VisitedLink::ParseRelativeLink(DataStreamReader* reader) {
  /* Assume previous parsing extracted TAG_RELATIVE_ENTRY from the reader.
   * So the next thing on the stream is the record size. */

  RelativeLink relative_link;
  size_t cookie_record_size  = reader->ReadSize();
  size_t record_end_position = reader->GetCurrentStreamPosition() +
                               cookie_record_size;
  while (!reader->IsEof() &&
         !reader->IsFailed() &&
         reader->GetCurrentStreamPosition() < record_end_position) {
    uint32_t tag = reader->ReadTag();
    switch (tag) {
      case TAG_RELATIVE_VISITED:
        relative_link.last_visited_ = reader->ReadContentWithSize<uint32_t>();
        break;
      case TAG_RELATIVE_NAME:
        relative_link.url_ = reader->ReadString();
        break;
      default:
        LOG(ERROR) << "Unexpected tag received: 0x" << std::hex << tag;
        return false;
    }
  }
  bool success = !reader->IsFailed()
      && relative_link.last_visited_
      && !relative_link.url_.empty();
  if (success)
    relative_links_.push_back(relative_link);
  return success;
}

GURL VisitedLink::GetUrl() const {
  return url_;
}

time_t VisitedLink::GetLastVisitedTime() const {
  return last_visited_;
}

const VisitedLink::RelativeLinks& VisitedLink::GetRelativeLinks() const {
  return relative_links_;
}

}  // namespace migration
}  // namespace common
}  // namespace opera
