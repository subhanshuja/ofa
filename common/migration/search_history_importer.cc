// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/search_history_importer.h"

#include "base/logging.h"
#include "base/memory/ref_counted.h"

#include "common/migration/tools/util.h"

namespace opera {
namespace common {
namespace migration {

SearchHistoryImporter::SearchHistoryImporter(
      scoped_refptr<SearchHistoryReceiver> historyReceiver)
      : history_receiver_(historyReceiver) {
}

SearchHistoryImporter::~SearchHistoryImporter() {
}

bool SearchHistoryImporter::Parse(std::istream* input) {
  std::string historyContent((std::istreambuf_iterator<char>(*input)),
                              std::istreambuf_iterator<char>());
  xmlDocPtr doc = xmlReadMemory(historyContent, "search_field_history.dat");

  if (!doc) {
    LOG(ERROR) << "Could not parse initial search history input as xml";
    return false;
  }

  XmlDocAnchor anchor(doc);

  xmlNode* root_element = xmlDocGetRootElement(doc);
  for (xmlNode* child = root_element->children; child; child = child->next) {
    std::string child_name = std::string(
        reinterpret_cast<const char*>(child->name));

    if (child_name == "text")
      continue;

    if (child_name != "search_entry") {
      LOG(ERROR) << "Search field history xml file is corrupted!"
                 << " Unknown node name.";
      return false;
    }

    int16_t attribute_count = 0;

    for (xmlAttr* attribute = child->properties;
        attribute;
        attribute = attribute->next) {
      std::string name =
          std::string(reinterpret_cast<const char*>(attribute->name));
      if (name == "search_id") {
        attribute_count++;
        continue;
      } else if (name == "term") {
        if (!attribute->children ||
          std::string(
              reinterpret_cast<const char*>(attribute->children->name)) !=
          "text") {
          LOG(ERROR) << "Search field history xml file is corrupted!"
                     << " Invalid attribute value.";
          return false;
        }

        std::string value(
            reinterpret_cast<const char*>(attribute->children->content));
        if (value.size() == 0) {
          LOG(ERROR) << "Search field history xml file is corrupted!"
                     << " Invalid attribute value.";
          return false;
        }

        searches_.push_back(value);
      } else {
        LOG(ERROR) << "Search field history xml file is corrupted!"
                   << " Unknown attribute name.";
        return false;
      }

      attribute_count++;
    }

    if (attribute_count != 2) {
      LOG(ERROR) << "Search field history xml file is corrupted!"
                 << "Unexpected number of attributes.";
      return false;
    }
  }

  return true;
}

void SearchHistoryImporter::Import(std::unique_ptr<std::istream> input,
    scoped_refptr<MigrationResultListener> listener) {
  bool result = Parse(input.get());

  if (result && history_receiver_) {
    history_receiver_->OnNewSearchQueries(searches_, listener);
    return;
  }
  listener->OnImportFinished(opera::SEARCH_HISTORY, result);
}

}  // namespace migration
}  // namespace common
}  // namespace opera
