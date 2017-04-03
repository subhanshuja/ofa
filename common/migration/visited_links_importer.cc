// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/visited_links_importer.h"

#include <vector>

#include "common/migration/vlinks/vlink4_tags.h"
#include "common/migration/tools/data_stream_reader.h"
#include "base/logging.h"

namespace opera {
namespace common {
namespace migration {

VisitedLinksImporter::VisitedLinksImporter(
    scoped_refptr<VisitedLinksListener> listener)
    : listener_(listener) {
}

VisitedLinksImporter::~VisitedLinksImporter() {
}

void VisitedLinksImporter::Import(
    std::unique_ptr<std::istream> input,
    scoped_refptr<MigrationResultListener> listener) {
  listener->OnImportFinished(opera::VISITED_LINKS, Parse(input.get()));
}

bool VisitedLinksImporter::Parse(std::istream* input) {
  DataStreamReader reader(input, true /* big endian */);
  if (reader.IsFailed())
    return false;

  std::vector<VisitedLink> visited_links;
  while (!reader.IsFailed() && !reader.IsEof()) {
    uint32_t tag = reader.ReadTag();
    if (tag == TAG_VISITED_FILE_ENTRY) {
      VisitedLink link;
      if (!link.Parse(&reader)) {
        LOG(ERROR) << "Could not parse visited link";
        return false;
      }
      visited_links.push_back(link);
    } else {
      LOG(ERROR) << "Unexpected tag 0x" << std::hex << tag << ", only expecting"
                 << TAG_VISITED_FILE_ENTRY << "(TAG_VISITED_FILE_ENTRY)";
      return false;
    }
  }
  if (listener_)
    listener_->OnVisitedLinksArrived(visited_links);
  return !reader.IsFailed();
}

}  // namespace migration
}  // namespace common
}  // namespace opera
