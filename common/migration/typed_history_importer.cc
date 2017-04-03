// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/typed_history_importer.h"

#include <ctime>

#include "common/migration/tools/util.h"
#include "base/logging.h"
#include "base/time/time.h"

using ::base::Time;

namespace opera {
namespace common {
namespace migration {

TypedHistoryImporter::TypedHistoryImporter(
      scoped_refptr<TypedHistoryReceiver> historyReceiver)
      : history_receiver_(historyReceiver) {
}

TypedHistoryImporter::~TypedHistoryImporter() {
}

bool TypedHistoryImporter::Parse(std::istream* input) {
  std::string historyContent((std::istreambuf_iterator<char>(*input)),
                              std::istreambuf_iterator<char>());
  xmlDocPtr doc = xmlReadMemory(historyContent, "typed_history.xml");

  if (!doc) {
    LOG(ERROR) << "Could not parse initial typed history input as xml";
    return false;
  }

  XmlDocAnchor anchor(doc);

  xmlNode* root_element = xmlDocGetRootElement(doc);
  for (xmlNode* child = root_element->children; child; child = child->next) {
    bool skip = false;

    std::string child_name(reinterpret_cast<const char*>(child->name));

    if (child_name == "text")
      continue;

    if (child_name != "typed_history_item") {
      LOG(ERROR) << "Typed history xml file is corrupted! Unknown node name.";
      return false;
    }

    TypedHistoryItem item;
    int16_t attribute_count = 0;

    for (xmlAttr* attribute = child->properties;
         attribute;
         attribute = attribute->next) {
      std::string name(reinterpret_cast<const char*>(attribute->name));

      if (!attribute->children ||
        std::string(reinterpret_cast<const char*>(
            attribute->children->name)) != "text") {
        LOG(ERROR) << "Typed history xml file is corrupted!"
                   << " Invalid attribute value.";
        return false;
      }

      if (name == "content") {
        item.url_ =
          std::string(reinterpret_cast<const char*>(
              attribute->children->content));
      } else if (name == "type") {
        std::string value(reinterpret_cast<const char*>(
            attribute->children->content));
        if (value == "text") {
          item.type_ = TypedHistoryItem::TYPED_HISTORY_ITEM_TYPE_TEXT;
        } else if (value == "selected") {
          item.type_ = TypedHistoryItem::TYPED_HISTORY_ITEM_TYPE_SELECTED;
        } else if (value == "search") {
          item.type_ = TypedHistoryItem::TYPED_HISTORY_ITEM_TYPE_SEARCH;
        } else if (value == "nickname") {
          skip = true;
          break;
        } else {
          LOG(ERROR) << "Typed history xml file is corrupted!"
                     << " Unknown item type.";
          return false;
        }
      } else if (name == "last_typed") {
        std::string value =
          std::string(reinterpret_cast<const char*>(
              attribute->children->content));
        if (value.size() != 20) {
          LOG(ERROR) << "Typed history xml file is corrupted!"
                     << " Unknown date format.";
          return false;
        }
        Time::Exploded time;
        time.year = atoi(value.substr(0, 4).c_str());
        time.month = atoi(value.substr(5, 2).c_str());
        time.day_of_month = atoi(value.substr(8, 2).c_str());
        time.hour = atoi(value.substr(11, 2).c_str());
        time.minute = atoi(value.substr(14, 2).c_str());
        time.second = atoi(value.substr(17, 2).c_str());
        time.millisecond = 0;
        // This shouldn't matter if only final value of time_t interests us.
        time.day_of_week = 0;

        if (!time.HasValidValues()) {
          LOG(ERROR) << "Typed history xml file is corrupted!"
                     << " Unknown date format.";
          return false;
        }

        item.last_typed_ = Time().FromUTCExploded(time).ToTimeT();
      } else {
        LOG(ERROR) << "Typed history xml file is corrupted!"
                   << " Unknown attribute name.";
        return false;
      }

      attribute_count++;
    }

    if (skip)
      continue;

    if (attribute_count != 3) {
      LOG(ERROR) << "Typed history xml file is corrupted!"
                 << " Unexpected number of attributes.";
      return false;
    }

    items_.push_back(item);
  }

  return true;
}

void TypedHistoryImporter::Import(
    std::unique_ptr<std::istream> input,
    scoped_refptr<MigrationResultListener> listener) {
  bool result = Parse(input.get());

  if (result && history_receiver_) {
    history_receiver_->OnNewHistoryItems(items_, listener);
    return;
  }

  listener->OnImportFinished(opera::TYPED_HISTORY, result);
}

}  // namespace migration
}  // namespace common
}  // namespace opera
