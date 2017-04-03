// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/network/response_parser.h"

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/values.h"

namespace opera {
namespace oauth2 {
ResponseParser::ResponseParser() {}
ResponseParser::~ResponseParser() {}

void ResponseParser::Expect(const std::string& key,
                            Type type,
                            Optional optional) {
  DCHECK_EQ(0u, items_.count(key));
  items_[key] = std::make_pair(type, optional);
}

bool ResponseParser::Parse(base::DictionaryValue* dict, ParseType type) {
  DCHECK(dict);
  base::DictionaryValue::Iterator iter(*dict);
  std::vector<std::string> found_keys;
  while (!iter.IsAtEnd()) {
    const auto& key = iter.key();
    if (items_.count(key) == 0) {
      VLOG(4) << "Unexpected key " << key;
      if (type == PARSE_STRICT) {
        Reset();
        return false;
      } else if (type == PARSE_SOFT) {
        iter.Advance();
        continue;
      } else {
        NOTREACHED() << type;
      }
    }
    const auto desc = items_.at(key);
    Optional optional = desc.second;
    base::Value::Type actual_type = iter.value().GetType();

    if (actual_type == base::Value::TYPE_NULL && optional == IS_OPTIONAL) {
      iter.Advance();
      continue;
    }

    bool bool_value;
    std::string str_value;
    int int_value;
    Type expected_type = desc.first;

    switch (expected_type) {
      case BOOLEAN:
        if (!iter.value().GetAsBoolean(&bool_value)) {
          VLOG(4) << "Wrong type for " << key;
          Reset();
          return false;
        }
        DCHECK_EQ(0u, bool_values_.count(key));
        bool_values_[key] = bool_value;
        break;
      case STRING:
        if (!iter.value().GetAsString(&str_value)) {
          VLOG(4) << "Wrong type for " << key;
          Reset();
          return false;
        }
        if (str_value.empty()) {
          VLOG(4) << "Empty value for " << key;
          Reset();
          return false;
        }
        DCHECK_EQ(0u, str_values_.count(key));
        str_values_[key] = str_value;
        break;
      case INTEGER:
        if (!iter.value().GetAsInteger(&int_value)) {
          VLOG(4) << "Wrong type for " << key;
          Reset();
          return false;
        }
        if (int_value <= 0) {
          VLOG(4) << "Zero or negative value for " << key;
          Reset();
          return false;
        }
        DCHECK_EQ(0u, int_values_.count(key));
        int_values_[key] = int_value;
        break;
      default:
        NOTREACHED();
    }
    found_keys.push_back(key);
    iter.Advance();
  }

  for (const auto& item : items_) {
    const auto& key = item.first;
    Type type = item.second.first;
    Optional optional = item.second.second;
    if (optional == IS_REQUIRED) {
      switch (type) {
        case BOOLEAN:
          if (bool_values_.count(key) == 0) {
            VLOG(4) << "Required key " << key << " not found";
            Reset();
            return false;
          }
          break;
        case STRING:
          if (str_values_.count(key) == 0) {
            VLOG(4) << "Required key " << key << " not found";
            Reset();
            return false;
          }
          break;
        case INTEGER:
          if (int_values_.count(key) == 0) {
            VLOG(4) << "Required key " << key << " not found";
            Reset();
            return false;
          }
          break;
        default:
          NOTREACHED();
      }
    }
  }
  return true;
}

bool ResponseParser::GetBoolean(const std::string& key) const {
  DCHECK_EQ(1u, bool_values_.count(key));
  return bool_values_.at(key);
}

int ResponseParser::GetInteger(const std::string& key) const {
  DCHECK_EQ(1u, int_values_.count(key));
  return int_values_.at(key);
}
std::string ResponseParser::GetString(const std::string& key) const {
  DCHECK_EQ(1u, str_values_.count(key));
  return str_values_.at(key);
}

bool ResponseParser::HasBoolean(const std::string& key) const {
  return (bool_values_.count(key) == 1u);
}

bool ResponseParser::HasInteger(const std::string& key) const {
  return (int_values_.count(key) == 1u);
}

bool ResponseParser::HasString(const std::string& key) const {
  return (str_values_.count(key) == 1u);
}

void ResponseParser::Reset() {
  bool_values_.clear();
  str_values_.clear();
  int_values_.clear();
}

}  // namespace oauth2
}  // namespace opera
