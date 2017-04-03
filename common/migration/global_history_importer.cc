// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/global_history_importer.h"

#include <cstdlib>

namespace opera {
namespace common {
namespace migration {

GlobalHistoryImporter::GlobalHistoryImporter(
      scoped_refptr<GlobalHistoryReceiver> historyReceiver)
      : history_receiver_(historyReceiver) {
}

GlobalHistoryImporter::~GlobalHistoryImporter() {
}

bool GlobalHistoryImporter::Parse(std::istream* input) {
  std::string line;
  GlobalHistoryItem current_item;
  int record_index = 0;

  while (std::getline(*input, line)) {
    // Mac needs text without trailing formatting characters for proper display
    // Remove now so that we do not have to do it every time a string is drawn.
    if (line.length() > 0)
      line.erase(line.find_last_not_of("\n\r")+1);

    switch (record_index++ % 4) {
    case 0:
      current_item.title_ = line;
      break;
    case 1:
      current_item.url_ = line;
      break;
    case 2:
      current_item.visit_time_ = std::atol(line.c_str());
      if (current_item.visit_time_ <= 0)
        return false;
      break;
    case 3:
      current_item.average_visit_interval_ = std::atoi(line.c_str());
      if (current_item.average_visit_interval_ == 0)
        return false;
      items_.push_back(current_item);
      break;
    }
  }

  return input->eof() && record_index % 4 == 0;
}

void GlobalHistoryImporter::Import(std::unique_ptr<std::istream> input,
    scoped_refptr<MigrationResultListener> listener) {
  bool result = Parse(input.get());

  if (result && history_receiver_) {
    history_receiver_->OnNewHistoryItems(items_, listener);
    return;
  }
  listener->OnImportFinished(opera::HISTORY, result);
}

}  // namespace migration
}  // namespace common
}  // namespace opera
